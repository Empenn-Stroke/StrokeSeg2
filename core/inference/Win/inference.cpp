#include "../inference.h"
#include "dml_ep_handler.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <cmath>
#include <array>

#define ENABLE_NPU_ADAPTER_ENUMERATION
#include <onnxruntime_cxx_api.h>
#include <dml_provider_factory.h>

using OrtFloat16 = Ort::Float16_t;

class Inference::Impl {
  public:
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::vector<Ort::Float16_t> m_inputBuffer;
    std::vector<Ort::Float16_t> m_outputBuffer;


    /* @brief Initialize the ONNX Runtime session with the specified model path. This method is
     *        called internally by loadModel() after loading the model.
     * @param modelPath The full path to the ONNX model file to load (including the .onnx
     *        extension).
     * @throws std::runtime_error if the model cannot be loaded or if there is an error initializing
     *         the ONNX Runtime session.
     */
    void init(const QString &modelPath) {
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "InferenceWin");

        DMLEpHandler::registerAvailableProviders(*m_env);

        Ort::SessionOptions options;
        OrtOpenVINOProviderOptions v_options;
        v_options.device_type = "NPU";

        try {
            options.AppendExecutionProvider_OpenVINO(v_options);
            qDebug() << "[SUCCESS] OpenVINO (NPU) added to session options.";
        } catch (const std::exception &e) {
            qDebug() << "[FALLBACK] OpenVINO failed, trying DirectML..." << e.what();

            bool deviceFound = attemptDML(options);
        }

        options.AddConfigEntry("session.set_denorm_as_zero", "1");

        try {
            m_session = std::make_unique<Ort::Session>(*m_env, modelPath.toStdWString().c_str(), options);
        } catch (const std::exception &e) {
            qDebug() << "Session Error:" << e.what();
            m_session.reset();
        }
    }


    /* @brief Attempt to configure the ONNX Runtime session to use DirectML (DML) as the execution
     * provider.
     *
     * @param options The SessionOptions object to configure with DML if available.
     * 
     * @return true if DML was successfully configured as the execution provider, false otherwise.
     */
    bool attemptDML(Ort::SessionOptions &options) {
        const OrtDmlApi *dmlApi = nullptr;
        if (Ort::GetApi().GetExecutionProviderApi(
                "DML", ORT_API_VERSION, reinterpret_cast<const void **>(&dmlApi)) != nullptr) {
            return false;
        }

        OrtDmlDeviceOptions devOptions;
        // Priorité 1 : NPU
        devOptions.Filter = OrtDmlDeviceFilter::Npu;
        devOptions.Preference = OrtDmlPerformancePreference::MinimumPower;
        if (dmlApi->SessionOptionsAppendExecutionProvider_DML2(
                static_cast<OrtSessionOptions *>(options), &devOptions) == nullptr) {
            qDebug() << "NPU targeted successfully via DML2 API.";
            return true;
        }

        // Priorité 2 : GPU
        devOptions.Filter = OrtDmlDeviceFilter::Gpu;
        devOptions.Preference = OrtDmlPerformancePreference::HighPerformance;
        if (dmlApi->SessionOptionsAppendExecutionProvider_DML2(
                static_cast<OrtSessionOptions *>(options), &devOptions) == nullptr) {
            qDebug() << "Discrete GPU targeted successfully via DML2 API.";
            return true;
        }

        qDebug() << "DML found but no NPU/GPU compatible, falling back to CPU.";
        return false;
    }

    
    /* @brief Slice a 3D image volume into overlapping patches based on the specified image size,
     * patch size, and step size.
     * @param image_size An array of three integers specifying the dimensions of the input image
     * volume (e.g., {width, height, depth}).
     * @param patch_size An array of three integers specifying the dimensions of each patch to
     * extract from the image volume (e.g., {patch_width, patch_height, patch_depth}).
     * @param step_size A float between 0 and 1 specifying the fraction of the patch size to use as
     * the step between patches. For example, a step_size of 0.5 means that patches will overlap by
     * 50%.
     * @return A vector of vectors of integers, where each inner vector contains the starting
     * indices (x, y, z) for a patch in the image volume. The outer vector contains one entry for
     * each patch.
     * @throws std::invalid_argument if any of the input parameters are invalid (e.g., negative
     * sizes, step_size not in (0, 1)).
     * @note The method calculates the starting indices for patches in a way that covers the entire
     *       image volume with overlapping patches. The last patches in each dimension may be
     * smaller than the specified patch size if they extend beyond the image boundaries. The method
     * does not actually extract or return the patch data; it only returns the starting indices for
     * where patches would be extracted from the image volume.
     */
    std::vector<std::vector<int>> sliceVolume(std::array<int, 3> image_size,
                                                               std::array<int, 3> patch_size,
                                                               float step_size) {
        std::vector<std::vector<int>> steps;

        // --- For each dimension, calculate the starting indices for patches ---
        for (int i = 0; i < 3; ++i) {
            std::vector<int> steps_here;
            int img_dim = image_size[i];
            int pat_dim = patch_size[i];

            // If the patch size is larger than the image dimension, we just take one patch starting
            // at 0 (after preprocessing, image should be at least the dimension of the patch, but
            // we keep this check for safety)
            if (img_dim <= pat_dim) {
                steps_here.push_back(0);

                // Otherwise, we calculate the steps based on the step size and ensure we cover the
                // entire dimension
            } else {
                float target_step = pat_dim * step_size;
                int num_steps = std::ceil((img_dim - pat_dim) / target_step) + 1;

                float actual_step = static_cast<float>(img_dim - pat_dim) / (num_steps - 1);

                for (int j = 0; j < num_steps; ++j) {
                    int start_idx = std::round(actual_step * j);

                    if (start_idx + pat_dim > img_dim) {
                        start_idx = img_dim - pat_dim;
                    }

                    if (steps_here.empty() || start_idx != steps_here.back()) {
                        steps_here.push_back(start_idx);
                    }
                }
            }
            steps.push_back(steps_here);
        }
        return steps;
    }

    /* @brief Compute a 3D Gaussian weighting function (kernel) based on the specified patch size,
     * sigma scale, and value scaling factor.
     * @param patch_size An array of three integers specifying the dimensions of the patch for which
     * to compute the Gaussian kernel (e.g., {patch_width, patch_height, patch_depth}).
     * @param sigma_scale A float specifying the scale factor for the standard deviation (sigma) of
     * the Gaussian function. The actual sigma will be calculated as sigma_scale multiplied by the
     *        corresponding dimension of the patch size. For example, if patch_size is {128, 128,
     * 128} and sigma_scale is 0.125, then sigma will be {16, 16, 16}.
     * @param value_scaling_factor A float specifying a scaling factor to apply to the values of the
     *        Gaussian kernel. This can be used to increase or decrease the overall magnitude of the
     * weights in the kernel.
     * @return An Eigen::Tensor<float, 3, Eigen::ColMajor> representing the computed 3D Gaussian
     * kernel with dimensions matching the specified patch size. The values in the tensor will be
     * scaled by the value_scaling_factor.
     * @throws std::invalid_argument if any of the input parameters are invalid (e.g., negative
     * sizes, non-positive sigma_scale).
     * @note The computed Gaussian kernel can be used for weighting patches during inference to give
     *       more importance to voxels near the center of the patch and less importance to voxels
     * near the edges. This can help reduce edge artifacts when combining predictions from
     * overlapping patches. The method does not perform any normalization on the kernel values; it
     * simply applies the specified value scaling factor after computing the Gaussian function. The
     * output tensor will have its dimensions ordered according to Eigen's column-major storage
     * format, which is consistent with how NIfTI data is stored on disk.
     */
    Eigen::Tensor<float, 3, Eigen::ColMajor> compute_gaussian(const std::array<int, 3> &patch_size,
                                                              float sigma_scale = 0.125f,
                                                              float value_scaling_factor = 10.0f) {
        const int nX = patch_size[0];
        const int nY = patch_size[1];
        const int nZ = patch_size[2];

        auto get_gaussian_1d_tensor = [](int size, float sigma) {
            Eigen::Tensor<float, 1, Eigen::ColMajor> kernel(size);
            float center = (size - 1) / 2.0f;
            for (int i = 0; i < size; ++i) {
                float x = static_cast<float>(i) - center;
                kernel(i) = std::exp(-(x * x) / (2.0f * sigma * sigma));
            }
            return kernel;
        };

        auto kX = get_gaussian_1d_tensor(nX, nX * sigma_scale);
        auto kY = get_gaussian_1d_tensor(nY, nY * sigma_scale);
        auto kZ = get_gaussian_1d_tensor(nZ, nZ * sigma_scale);

        Eigen::array<int, 3> reshapeX = {nX, 1, 1};
        Eigen::array<int, 3> bcastX = {1, nY, nZ};

        Eigen::array<int, 3> reshapeY = {1, nY, 1};
        Eigen::array<int, 3> bcastY = {nX, 1, nZ};

        Eigen::array<int, 3> reshapeZ = {1, 1, nZ};
        Eigen::array<int, 3> bcastZ = {nX, nY, 1};

        Eigen::Tensor<float, 3, Eigen::ColMajor> gaussian_map =
            kX.reshape(reshapeX).broadcast(bcastX) * kY.reshape(reshapeY).broadcast(bcastY) *
            kZ.reshape(reshapeZ).broadcast(bcastZ);

        Eigen::Tensor<float, 0, Eigen::ColMajor> max_tensor = gaussian_map.maximum();
        float max_val = max_tensor(0);

        if (max_val > 0.0f) {
            float factor = value_scaling_factor / max_val;
            float min_val = factor * 1e-4f;

            gaussian_map = (gaussian_map * factor).cwiseMax(min_val);
        }

        return gaussian_map;
    }

};

Inference::Inference() : m_pimpl(std::make_unique<Impl>()) {}
Inference::~Inference() = default;

bool Inference::loadModel(const QString &modelPath) {
    try {
        m_pimpl->init(modelPath);
        return true;
    } catch (const std::exception &e) {
        qDebug() << "Load error:" << e.what();
        return false;
    }
}


/* @brief Run inference on the specified input image using the loaded ONNX model and return the
 *        output as a NiftiVolume.
 * @param modelPath The full path to the ONNX model file to use for inference (including the .onnx
 *        extension).
 * @param imagePath The full path to the input image file (e.g., a NIfTI file) to run inference on.
 * @param destinationPath The full path where the output NIfTI file should be saved (including the
 *        .nii or .nii.gz extension).
 * @param inputName The name of the input tensor in the ONNX model that corresponds to the input
 *        image data.
 * @param outputName The name of the output tensor in the ONNX model that contains the inference
 *        results.
 * @return A NiftiVolume containing the inference results, which can also be saved to disk at the
 *         specified destinationPath.
 * @throws std::runtime_error if there is an error loading the model, reading the input image,
 *         running inference, or saving the output.
 * @note This method assumes that loadModel() has already been called to load and initialize the
 *       ONNX Runtime session with the specified model. The method will read the input image,
 * preprocess it as needed, run it through the ONNX model using the specified input and output
 * tensor names, postprocess the output as needed, and return it as a NiftiVolume. The output will
 * also be saved to disk at destinationPath.
 */
NiftiVolume Inference::run(const QString &modelPath, const QString &imagePath,
                           const QString &destinationPath, const QString &inputName,
                           const QString &outputName) {

    if (!QFile::exists(modelPath) || !QFile::exists(imagePath)) {
        qDebug() << modelPath;
        qDebug() << "Image or model not found !";
        return {};
    }

    // --- Prepare input and output names for ONNX Runtime ---
    std::string inputNameStr = inputName.toStdString();
    std::string outputNameStr = outputName.toStdString();
    const char *inputNames[] = {inputNameStr.c_str()};
    const char *outputNames[] = {outputNameStr.c_str()};

    qDebug() << "Loading image from :" << imagePath;
    NiftiVolume nv = NiftiVolume::loadNifti(imagePath);

    int X = nv.data.dimension(0);
    int Y = nv.data.dimension(1);
    int Z = nv.data.dimension(2);
    int C = nv.data.dimension(3);

    qDebug() << "Shape (X:" << X << ", Y:" << Y << ", Z:" << Z << ", C:" << C << ")";

    // --- Initialize ONNX Runtime session and tensors for future computation ---
    if (!m_pimpl->m_session) {
        m_pimpl->init(modelPath);
    }

    Ort::IoBinding io_binding(*m_pimpl->m_session);

    Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor> full_volume_f16 = nv.data.cast<OrtFloat16>();
    nv.data = Eigen::Tensor<float, 4, Eigen::ColMajor>();

    std::array<int, 3> patch_size = {128, 128, 128};
    auto steps = m_pimpl->sliceVolume({X, Y, Z}, patch_size, 0.5f);
    auto gaussian = m_pimpl->compute_gaussian(patch_size);

    int num_classes = 2;
    Eigen::Tensor<float, 4, Eigen::ColMajor> output_accum(X, Y, Z, num_classes);
    Eigen::Tensor<float, 4, Eigen::ColMajor> norm_map(X, Y, Z, 1);
    output_accum.setZero();
    norm_map.setZero();

    Eigen::array<int, 4> bcast_out = {1, 1, 1, num_classes};
    Eigen::array<int, 4> bcast_norm = {1, 1, 1, 1};
    Eigen::Tensor<float, 4, Eigen::ColMajor> g_out =
        gaussian.reshape(Eigen::array<int, 4>{128, 128, 128, 1}).broadcast(bcast_out);
    Eigen::Tensor<float, 4, Eigen::ColMajor> g_norm =
        gaussian.reshape(Eigen::array<int, 4>{128, 128, 128, 1}).broadcast(bcast_norm);

    // --- Prepare memory info once outside the loop ---
    Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    // Pre allocating memory to avoid
    const int64_t patch_elements = 128 * 128 * 128 * C;
    std::vector<OrtFloat16> input_buffer(patch_elements);

    const int64_t output_elements = 128 * 128 * 128 * num_classes;
    std::vector<OrtFloat16> output_buffer(output_elements);

    std::vector<int64_t> input_shape = {1, C, 128, 128, 128};
    std::vector<int64_t> output_shape = {1, num_classes, 128, 128, 128};

    Ort::Value input_tensor = Ort::Value::CreateTensor<OrtFloat16>(
        mem_info, input_buffer.data(), input_buffer.size(), input_shape.data(), input_shape.size());
    Ort::Value output_tensor =
        Ort::Value::CreateTensor<OrtFloat16>(mem_info, output_buffer.data(), output_buffer.size(),
                                             output_shape.data(), output_shape.size());

    io_binding.BindOutput(outputNameStr.c_str(), output_tensor);

    qDebug() << "Running inference on patches...";

    int patch_cpt = 0;
    int total_patches = steps[0].size() * steps[1].size() * steps[2].size();

    QElapsedTimer patch_timer;
    patch_timer.start();

    for (int x : steps[0]) {
        for (int y : steps[1]) {
            for (int z : steps[2]) {
                patch_cpt++;
                qDebug() << "Processing patch (" << patch_cpt << "/" << total_patches
                         << ") at (X:" << x << ", Y:" << y << ", Z:" << z << ")";

                Eigen::TensorMap<Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor>> input_map(
                    input_buffer.data(), 128, 128, 128, C);

                // --- Extract patch and prepare input tensor ---
                Eigen::array<int, 4> offset = {x, y, z, 0};
                Eigen::array<int, 4> extent = {128, 128, 128, C};

                input_map = full_volume_f16.slice(offset, extent);

                // --- Create input tensor for ONNX Runtime ---
                std::vector<int64_t> patch_shape = {1, C, 128, 128, 128};

                io_binding.BindInput(inputNameStr.c_str(), input_tensor);

                // --- Run inference ---
                // auto output_tensors = m_session->Run(Ort::RunOptions{nullptr}, inputNames,
                //                                     &input_tensor, 1, outputNames, 1);

                m_pimpl->m_session->Run(Ort::RunOptions{nullptr}, io_binding);

                Eigen::TensorMap<Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor>> output_map_res(
                    output_buffer.data(), 128, 128, 128, num_classes);

                // --- Accumulate results ---
                output_accum.slice(Eigen::array<int, 4>{x, y, z, 0},
                                   Eigen::array<int, 4>{128, 128, 128, num_classes}) +=
                    output_map_res.cast<float>() * g_out;

                norm_map.slice(Eigen::array<int, 4>{x, y, z, 0},
                               Eigen::array<int, 4>{128, 128, 128, 1}) += g_norm;

                qDebug() << "Patch processed in" << static_cast<float>(patch_timer.elapsed()) / 1000
                         << "s";
                patch_timer.restart();
            }
        }
    }

    qDebug() << "Normalizing...";
    output_accum /= norm_map.broadcast(Eigen::array<int, 4>{1, 1, 1, num_classes});

    qDebug() << "Extracting class 1 and reshaping to 3D...";

    Eigen::Tensor<float, 3, Eigen::ColMajor> final_tensor_3d = output_accum.chip(1, 3);

    NiftiVolume outputVol = nv;
    outputVol.data = final_tensor_3d.reshape(Eigen::array<int, 4>{X, Y, Z, 1});

    qDebug() << "[FINAL CHECK] Dimensions to save:" << outputVol.data.dimension(0) << "x"
             << outputVol.data.dimension(1) << "x" << outputVol.data.dimension(2) << "x"
             << outputVol.data.dimension(3);

    NiftiVolume::saveNifti(destinationPath, outputVol);

    return outputVol;
}