#include "inferenceengine.h"
#include <algorithm>
#include <iostream>
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <vector>

#ifdef Q_OS_WIN
#include <dml_provider_factory.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#pragma comment(lib, "dxgi.lib")
#endif

/* @brief Check if a discrete GPU is present on the system by enumerating the available graphics
 * adapters using DirectX Graphics Infrastructure (DXGI) on Windows. The function looks for adapters
 * that are not flagged as software (i.e., not WARP) and have a dedicated video memory greater
 * than 1.5 GB, which is a common threshold for considering an adapter as a discrete GPU. If such an
 * adapter is found, the function returns true; otherwise, it returns false. This check is used to
 * determine whether to use GPU acceleration for ONNX Runtime inference.
 */
bool isDiscreteGPUPresent() {
#if defined(Q_OS_WIN)
    Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)factory.GetAddressOf())))
        return false;

    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; factory->EnumAdapters1(i, adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND;
         ++i) {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        // Ignore software adapters
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            continue;

        // If GPU has more than 1.5G of VRAM, we use the GPU
        if (desc.DedicatedVideoMemory > 1500 * 1024 * 1024) {
            qDebug() << "GPU Dédié détecté :" << QString::fromWCharArray(desc.Description);
            return true;
        }
    }
#elif defined(Q_OS_LINUX)
    //TO BE TESTED
    QProcess process;
    process.start("lspci", {"-v"});
    if (!process.waitForFinished())
        return false;

    QString output = process.readAllStandardOutput();

    // We look for common discrete GPU identifiers in the lspci output. This is a heuristic and may
    // not be perfect.
    if (output.contains("NVIDIA") || output.contains("Radeon") || output.contains("Navi")) {
        // Check if the GPU has dedicated VRAM (not shared with system memory). This is a heuristic
        // based on the presence of "VRAM" or "memory" in the lspci output for the GPU. It is not a
        // perfect check, but it can help distinguish discrete GPUs from integrated ones.
        if (output.contains("VRAM") || output.contains("memory")) {
            return true;
        }
    }
elif defined(Q_OS_MAC)
    //TO BE TESTED
    // macOS typically uses integrated GPUs, but some models have discrete GPUs. We can check for
    // the presence of a discrete GPU using the system_profiler command and looking for "Discrete
    // GPU" in the output. This is a heuristic and may not be perfect.
    QProcess process;
    process.start("system_profiler", {"SPDisplaysDataType"});
    if (!process.waitForFinished())
        return false;
    QString output = process.readAllStandardOutput();
    if (output.contains("Discrete GPU")) {
        return true;
    }
#endif 
    return false;
}

using OrtFloat16 = Ort::Float16_t;

InferenceEngine::InferenceEngine() {}


/* @brief Get the list of available ONNX models in the predefined models directory.
 *        The models should have a .onnx extension.
 * @return QStringList containing the names of the available models (without path).
 * @throws std::runtime_error if the models directory cannot be accessed.
 */
QStringList InferenceEngine::getAvailableModels() 
{
    QString programDataPath = qgetenv("PROGRAMDATA");
    if (programDataPath.isEmpty())
        programDataPath = "C:/ProgramData";

    QDir modelsDir(programDataPath + "/StrokeSeg/Models");
    return modelsDir.entryList({"*.onnx"}, QDir::Files);
}

/* @brief Load the specified ONNX model from the predefined models directory and
 *        initialize the ONNX Runtime session.
 * @param modelName The name of the model to load (without path).
 * @throws std::runtime_error if the model cannot be found or loaded.
 * @throws Ort::Exception if there is an error initializing the ONNX Runtime session.
 * @note This method must be called before running inference with the run() method.
 *       The model will be loaded from the path: getModelsPath() + "/" + modelName + ".onnx"
 *       For example, if modelName is "stroke_segmentation", the method will attempt
 *       to load "C:/ProgramData/StrokeSeg/Models/stroke_segmentation.onnx"
 *       (assuming getModelsPath() returns "C:/ProgramData/StrokeSeg/Models").
 *       The method will also initialize the ONNX Runtime session with the loaded model, which
 *       is required for running inference.
 
 */
bool InferenceEngine::loadModel(const QString &modelName) 
{
    QString fullPath = getModelsPath() + modelName;

    // --- Check if the model file exists and initialize the session ---
    if (QFile::exists(fullPath)) {
        this->initSession(fullPath);
        return true;
    } else {
        qDebug() << "Model file not found:" << fullPath;
        return false;
    }
}

/* @brief Initialize the ONNX Runtime session with the specified model path. This method is
 *        called internally by loadModel() after loading the model.
 * @param modelPath The full path to the ONNX model file to load (including the .onnx
 *        extension).
 * @throws std::runtime_error if the model cannot be loaded or if there is an error initializing
 *         the ONNX Runtime session.
 */
void InferenceEngine::initSession(const QString &modelPath) {
    if (!m_env) {
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "Inference");
    }

    Ort::SessionOptions sessionOptions;
    bool deviceFound = false;

    auto providers = Ort::GetAvailableProviders();
    
#if defined(Q_OS_WIN)
    bool hasDML = std::find(providers.begin(), providers.end(), "DmlExecutionProvider") != providers.end();
    Microsoft::WRL::ComPtr<IDXGIFactory6> factory;
    // First, we look for an NPU (Neural Processing Unit) if available, as it can provide better
    // performance for AI workloads. Intel refers to their NPU as "AI Boost" or "NPU" in the adapter
    // description, so we use a heuristic to identify it.
    if (hasDML && SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory6), (void **)factory.GetAddressOf()))) {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
        // We use EnumAdapterByGpuPreference with DXGI_GPU_PREFERENCE_MINIMUM_POWER to prioritize
        // low-power devices like NPUs. This allows us to find the NPU even if a discrete GPU is
        // present, as the NPU will typically be listed with a lower power preference than the
        // discrete GPU.
        for (UINT i = 0; factory->EnumAdapterByGpuPreference(
                             i, DXGI_GPU_PREFERENCE_MINIMUM_POWER, __uuidof(IDXGIAdapter1),
                             (void **)adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND;
             ++i) {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            QString name = QString::fromWCharArray(desc.Description);

            qDebug() << "Checking adapter:" << name;

            // Heuristic for NPU: Intel refers to it as "AI Boost" or "NPU"
            if (name.contains("NPU") || name.contains("AI Boost")) {
                qDebug() << "Priority 1: NPU detected and selected:" << name;
                Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, i));
                deviceFound = true;
                break;
            }
        }

        // If no NPU was found, we look for a discrete GPU as a fallback. We use the same heuristic
        // as in isDiscreteGPUPresent() to identify discrete GPUs based on their flags and dedicated
        // video memory.
        if (hasDML && !deviceFound) {
            for (UINT i = 0; factory->EnumAdapterByGpuPreference(
                                 i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, __uuidof(IDXGIAdapter1),
                                 (void **)adapter.GetAddressOf()) != DXGI_ERROR_NOT_FOUND;
                 ++i) {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) &&
                    desc.DedicatedVideoMemory > 1500 * 1024 * 1024) {
                    qDebug() << "Priority 2: Discrete GPU detected and selected:"
                             << QString::fromWCharArray(desc.Description);
                    Ort::ThrowOnError(
                        OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, i));
                    deviceFound = true;
                    break;
                }
            }
        }
    }
#elif defined(Q_OS_LINUX)
    // TO BE TESTED
    auto availableProviders = Ort::GetAvailableProviders();

    // --- 1. Attempt Intel NPU (OpenVINO) ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "OpenVINOExecutionProvider") != availableProviders.end()) {
        try {
            OrtOpenVINOProviderOptions options;
            options.device_type = "NPU"; // Target specifically the NPU
            // The following call will throw or fail if the NPU is not physically present or drivers
            // are missing
            sessionOptions.AppendExecutionProvider_OpenVINO(options);

            // Validate by trying to create a dummy session or checking internal API
            qDebug() << "Priority 1: OpenVINO NPU successfully attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "OpenVINO NPU requested but not available on this system:" << e.what();
        }
    }

    // --- 2. Attempt AMD NPU (VitisAI) ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "VitisAIExecutionProvider") != availableProviders.end()) {
        try {
            std::unordered_map<std::string, std::string> vitis_options;
            vitis_options["config_file"] = "/etc/vaip_config.json";

            sessionOptions.AppendExecutionProvider_VitisAI(vitis_options);
            qDebug() << "Priority 2: VitisAI NPU successfully attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "VitisAI NPU requested but not available:" << e.what();
        }
    }

    // --- 3. Attempt discrete GPU TensorRT RTX ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "TensorrtExecutionProvider") != availableProviders.end()) {
        try {
            OrtTensorRTProviderOptions trt_options{};
            trt_options.device_id = 0;
            trt_options.trt_fp16_enable = 1; // Critical for RTX performance
            trt_options.trt_engine_cache_enable = 1;
            trt_options.trt_engine_cache_path = "/tmp/trt_cache";

            sessionOptions.AppendExecutionProvider_TensorRT_V2(trt_options);

            // TensorRT requires CUDA as a fallback/helper
            OrtCUDAProviderOptions cuda_options{};
            sessionOptions.AppendExecutionProvider_CUDA(cuda_options);

            qDebug() << "Priority 3: Nvidia RTX (TensorRT) attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "TensorRT failed:" << e.what();
        }
    }

    // --- 4. Attempt discrete GPU via CUDA (for older GPUs) ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "CUDAExecutionProvider") != availableProviders.end()) {
        try {
            OrtCUDAProviderOptions cuda_options{};
            sessionOptions.AppendExecutionProvider_CUDA(cuda_options);
            qDebug() << "Priority 3: Nvidia GPU detected and CUDA Provider attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "CUDA initialization failed:" << e.what();
        }
    }

    // --- 5. Attempt discrete GPU via MIGraphX (for AMD GPUs) ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "MIGraphXExecutionProvider") != availableProviders.end()) {
        try {
            OrtMIGraphXProviderOptions mig_options{};
            sessionOptions.AppendExecutionProvider_MIGraphX(mig_options);
            qDebug() << "Priority 3: AMD GPU detected and MIGraphX Provider attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "MIGraphX initialization failed:" << e.what();
        }
    }

    // --- 6. Attempt discrete GPU via Rocm (for AMD GPUs) ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "RocmExecutionProvider") != availableProviders.end()) {
        try {
            OrtRocmProviderOptions roc_options{};
            sessionOptions.AppendExecutionProvider_Rocm(roc_options);
            qDebug() << "Priority 3: AMD GPU detected and Rocm Provider attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "Rocm initialization failed:" << e.what();
        }
    }

    // --- 7. Attempt discrete GPU via OpenVINO (for Intel GPUs) ---
    if (!deviceFound && std::find(availableProviders.begin(), availableProviders.end(),
                                  "OpenVINOExecutionProvider") != availableProviders.end()) {
        try {
            OrtOpenVINOProviderOptions options;
            options.device_type = "GPU"; // Target discrete GPU if NPU not found
            sessionOptions.AppendExecutionProvider_OpenVINO(options);
            qDebug() << "Priority 4: OpenVINO GPU successfully attached.";
            deviceFound = true;
        } catch (const std::exception &e) {
            qDebug() << "OpenVINO GPU initialization failed:" << e.what();
        }
    }


#elif defined(Q_OS_MAC)

#endif

    if (!deviceFound) {
        qDebug() << "No NPU or discrete GPU found. Using default CPU execution provider.";
        sessionOptions.SetIntraOpNumThreads(std::thread::hardware_concurrency());
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    }

    sessionOptions.AddConfigEntry("session.set_denorm_as_zero", "1");

    try {
        m_session = std::make_unique<Ort::Session>(*m_env, modelPath.toStdWString().c_str(),
                                                   sessionOptions);
    } catch (const std::exception &e) {
        qDebug() << "Session Error:" << e.what();
        m_session.reset();
    }
}

/* @brief Slice a 3D image volume into overlapping patches based on the specified image size, patch
 *        size, and step size.
 * @param image_size An array of three integers specifying the dimensions of the input image volume
 *        (e.g., {width, height, depth}).
 * @param patch_size An array of three integers specifying the dimensions of each patch to extract
 *        from the image volume (e.g., {patch_width, patch_height, patch_depth}).
 * @param step_size A float between 0 and 1 specifying the fraction of the patch size to use as the
 *        step between patches. For example, a step_size of 0.5 means that patches will overlap by 50%.
 * @return A vector of vectors of integers, where each inner vector contains the starting indices
 *         (x, y, z) for a patch in the image volume. The outer vector contains one entry for each patch.
 * @throws std::invalid_argument if any of the input parameters are invalid (e.g., negative sizes,
 *         step_size not in (0, 1)).
 * @note The method calculates the starting indices for patches in a way that covers the entire
 *       image volume with overlapping patches. The last patches in each dimension may be smaller than the
 *       specified patch size if they extend beyond the image boundaries. The method does not actually
 *       extract or return the patch data; it only returns the starting indices for where patches would be
 *       extracted from the image volume.
 */
std::vector<std::vector<int>> InferenceEngine::sliceVolume(std::array<int, 3> image_size,
                                                           std::array<int, 3> patch_size,
                                                           float step_size) 
{
    std::vector<std::vector<int>> steps;

    // --- For each dimension, calculate the starting indices for patches ---
    for (int i = 0; i < 3; ++i) {
        std::vector<int> steps_here;
        int img_dim = image_size[i];
        int pat_dim = patch_size[i];

        // If the patch size is larger than the image dimension, we just take one patch starting at 0 
        // (after preprocessing, image should be at least the dimension of the patch, but we keep
        // this check for safety)
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

/* @brief Compute a 3D Gaussian weighting function (kernel) based on the specified patch size, sigma
 *        scale, and value scaling factor.
 * @param patch_size An array of three integers specifying the dimensions of the patch for which to
 *        compute the Gaussian kernel (e.g., {patch_width, patch_height, patch_depth}).
 * @param sigma_scale A float specifying the scale factor for the standard deviation (sigma) of the
 *        Gaussian function. The actual sigma will be calculated as sigma_scale multiplied by the
 *        corresponding dimension of the patch size. For example, if patch_size is {128, 128, 128} and
 *        sigma_scale is 0.125, then sigma will be {16, 16, 16}.
 * @param value_scaling_factor A float specifying a scaling factor to apply to the values of the
 *        Gaussian kernel. This can be used to increase or decrease the overall magnitude of the weights in
 *        the kernel.
 * @return An Eigen::Tensor<float, 3, Eigen::ColMajor> representing the computed 3D Gaussian kernel
 *         with dimensions matching the specified patch size. The values in the tensor will be scaled by the
 *         value_scaling_factor.
 * @throws std::invalid_argument if any of the input parameters are invalid (e.g., negative sizes,
 *         non-positive sigma_scale).
 * @note The computed Gaussian kernel can be used for weighting patches during inference to give
 *       more importance to voxels near the center of the patch and less importance to voxels near the
 *       edges. This can help reduce edge artifacts when combining predictions from overlapping patches.
 *       The method does not perform any normalization on the kernel values; it simply applies the
 *       specified value scaling factor after computing the Gaussian function. The output tensor will have
 *       its dimensions ordered according to Eigen's column-major storage format, which is consistent with
 *       how NIfTI data is stored on disk.
 */
Eigen::Tensor<float, 3, Eigen::ColMajor>
InferenceEngine::compute_gaussian(const std::array<int, 3> &patch_size, float sigma_scale,
                                  float value_scaling_factor) 
{
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

    Eigen::Tensor<float, 3, Eigen::ColMajor> gaussian_map = kX.reshape(reshapeX).broadcast(bcastX) *
                                                            kY.reshape(reshapeY).broadcast(bcastY) *
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
 *       ONNX Runtime session with the specified model. The method will read the input image, preprocess
 *       it as needed, run it through the ONNX model using the specified input and output tensor names,
 *       postprocess the output as needed, and return it as a NiftiVolume. The output
 *       will also be saved to disk at destinationPath.
 */
NiftiVolume InferenceEngine::run(const QString &modelPath, const QString &imagePath,
                                 const QString &destinationPath, const QString &inputName,
                                 const QString &outputName) 
{
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
    if (!m_session) {
        initSession(modelPath);
    }

    Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor> full_volume_f16 = nv.data.cast<OrtFloat16>();
    nv.data = Eigen::Tensor<float, 4, Eigen::ColMajor>();

    std::array<int, 3> patch_size = {128, 128, 128};
    auto steps = sliceVolume({X, Y, Z}, patch_size, 0.5f);
    auto gaussian = compute_gaussian(patch_size);

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

                // --- Extract patch and prepare input tensor ---
                Eigen::array<int, 4> offset = {x, y, z, 0};
                Eigen::array<int, 4> extent = {128, 128, 128, C};

                Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor> patch_col =
                    full_volume_f16.slice(offset, extent);
                Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor> patch_onnx_ready =
                    patch_col.cast<OrtFloat16>();

                // --- Create input tensor for ONNX Runtime ---
                std::vector<int64_t> patch_shape = {1, C, 128, 128, 128};

                Ort::Value input_tensor = Ort::Value::CreateTensor<OrtFloat16>(
                    mem_info, patch_onnx_ready.data(), patch_onnx_ready.size(), patch_shape.data(),
                    patch_shape.size());

                // --- Run inference ---
                auto output_tensors = m_session->Run(Ort::RunOptions{nullptr}, inputNames,
                                                     &input_tensor, 1, outputNames, 1);

                // --- Process output tensor ---
                OrtFloat16 *raw_data = output_tensors[0].GetTensorMutableData<OrtFloat16>();

                Eigen::TensorMap<Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor>> output_map(
                    raw_data, 128, 128, 128, num_classes);

                Eigen::Tensor<float, 4, Eigen::ColMajor> pred_col = output_map.cast<float>();

                // --- Accumulate results ---
                output_accum.slice(Eigen::array<int, 4>{x, y, z, 0},
                                   Eigen::array<int, 4>{128, 128, 128, num_classes}) +=
                    pred_col * g_out;

                norm_map.slice(Eigen::array<int, 4>{x, y, z, 0},
                               Eigen::array<int, 4>{128, 128, 128, 1}) += g_norm;

                qDebug() << "Patch processed in" << static_cast<float>(patch_timer.elapsed()) / 1000 << "s";
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