// SPDX-License-Identifier: AGPL-3.0-or-later

#include "inference.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>

#include "model.h"
#include "managers/progressManager.h"

/* @brief Run inference on the specified input image using the loaded ONNX model and return the
 *        output as a NiftiVolume.
 * @param modelPath The full path to the ONNX model file to use for inference (including the .onnx
 *        extension).
 * @param image The ptr to the NiftiVolume image.
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

NiftiVolume Inference::run(std::shared_ptr<Model> model, NiftiVolume &image,
                           const QString &destinationPath, const QString &inputName,
                           const QString &outputName, std::array<int, 3> patchSize,
                           int numClasses)
{
    if (!model || !model->isValid()) 
    {
        qDebug() << "Erreur d'inférence : Le modèle fourni est nul ou invalide.";
        return {};
    }

    const int X = image.data.dimension(0);
    const int Y = image.data.dimension(1);
    const int Z = image.data.dimension(2);
    const int C = image.data.dimension(3);

    // FP16 Cast 
    Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor> full_volume_f16 = image.data.cast<Eigen::half>();
    image.data = Eigen::Tensor<float, 4, Eigen::ColMajor>();

    auto steps = sliceVolume({X, Y, Z}, patchSize, 0.5f);
    auto gaussian = compute_gaussian(patchSize);

    Eigen::Tensor<float, 4, Eigen::ColMajor> output_accum(X, Y, Z, numClasses);
    Eigen::Tensor<float, 4, Eigen::ColMajor> norm_map(X, Y, Z, 1);
    output_accum.setZero();
    norm_map.setZero();

    // mask broadcast
    Eigen::Tensor<float, 4, Eigen::ColMajor> g_out =
        gaussian.reshape(Eigen::array<int, 4>{patchSize[0], patchSize[1], patchSize[2], 1})
            .broadcast(Eigen::array<int, 4>{1, 1, 1, numClasses});
    Eigen::Tensor<float, 4, Eigen::ColMajor> g_norm =
        gaussian.reshape(Eigen::array<int, 4>{patchSize[0], patchSize[1], patchSize[2], 1})
            .broadcast(Eigen::array<int, 4>{1, 1, 1, 1});

    // buffer config for Model abstraction layer (IoBinding requires pre-allocated buffers)
    const Ort::MemoryInfo &mem_info = model->getMemoryInfo();

    const int64_t patch_elements = patchSize[0] * patchSize[1] * patchSize[2] * C;
    std::vector<Ort::Float16_t> input_buffer(patch_elements);

    const int64_t output_elements = patchSize[0] * patchSize[1] * patchSize[2] * numClasses;
    std::vector<Ort::Float16_t> output_buffer(output_elements);

    std::vector<int64_t> input_shape = {1, C, patchSize[0], patchSize[1], patchSize[2]};
    std::vector<int64_t> output_shape = {1, numClasses, patchSize[0], patchSize[1], patchSize[2]};

    // data layout transformation (Nifti/ONNX format) + creation of Ort tensors directly on
    // pre-allocated buffers
    Ort::Value input_tensor = Ort::Value::CreateTensor<Ort::Float16_t>(
        mem_info, input_buffer.data(), input_buffer.size(), input_shape.data(), input_shape.size());

    Ort::Value output_tensor = Ort::Value::CreateTensor<Ort::Float16_t>(
        mem_info, output_buffer.data(), output_buffer.size(), output_shape.data(),
        output_shape.size());

    // tensors binding via names
    std::string inName = inputName.toStdString();
    std::string outName = outputName.toStdString();
    std::vector<const char *> inputNames = {inName.c_str()};
    std::vector<const char *> outputNames = {outName.c_str()};
    std::vector<Ort::Value> inputTensors;
    inputTensors.push_back(std::move(input_tensor));
    std::vector<Ort::Value> outputTensors;
    outputTensors.push_back(std::move(output_tensor));

    int patch_cpt = 0;
    int total_patches = static_cast<int>(steps[0].size() * steps[1].size() * steps[2].size());

    for (int x : steps[0])
    {
        for (int y : steps[1])
        {
            for (int z : steps[2])
            {
                patch_cpt++;

                QString status =
                    QString("Processing patch %1/%2").arg(patch_cpt).arg(total_patches);
                ProgressManager::instance().report(41, 43, (patch_cpt * 100 / total_patches),
                                                   &status);

                Eigen::array<int, 4> offset = {x, y, z, 0};
                Eigen::array<int, 4> extent = {patchSize[0], patchSize[1], patchSize[2], C};

                // 1. Extraction and copy into IoBinding buffer (with FP16 cast)
                Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor> patch =
                    full_volume_f16.slice(offset, extent);
                Eigen::TensorMap<Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor>> input_map(
                    reinterpret_cast<Eigen::half *>(input_buffer.data()), patchSize[0],
                    patchSize[1], patchSize[2], C);
                input_map = patch;

                // 2. Inference via the Model abstraction layer (total independence here)
                model->run(inputNames, inputTensors, outputNames, outputTensors);

                // 3. Retrieval and reassembly
                Eigen::TensorMap<Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor>> output_map_res(
                    reinterpret_cast<Eigen::half *>(output_buffer.data()), patchSize[0],
                    patchSize[1], patchSize[2], numClasses);

                Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor> out_patch = output_map_res;

                output_accum.slice(
                    Eigen::array<int, 4>{x, y, z, 0},
                    Eigen::array<int, 4>{patchSize[0], patchSize[1], patchSize[2], numClasses}) +=
                    out_patch.cast<float>() * g_out;

                norm_map.slice(Eigen::array<int, 4>{x, y, z, 0},
                               Eigen::array<int, 4>{patchSize[0], patchSize[1], patchSize[2], 1}) +=
                    g_norm;
            }
        }
    }

    output_accum /= norm_map.broadcast(Eigen::array<int, 4>{1, 1, 1, numClasses});

    NiftiVolume outputVol = image;
    outputVol.data = output_accum;

    if (!destinationPath.isEmpty()) {
        NiftiVolume::saveNifti(destinationPath, outputVol);
    }

    return outputVol;
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
inline std::vector<std::vector<int>> Inference::sliceVolume(std::array<int, 3> image_size,
                                                            std::array<int, 3> patch_size,
                                                            float step_size) 
{
    std::vector<std::vector<int>> steps;
    for (int i = 0; i < 3; ++i) 
    {
        std::vector<int> steps_here;
        int img_dim = image_size[i];
        int pat_dim = patch_size[i];
        if (img_dim <= pat_dim) 
        {
            steps_here.push_back(0);
        } 
        else 
        {
            float target_step = pat_dim * step_size;
            int num_steps = std::ceil((img_dim - pat_dim) / target_step) + 1;
            float actual_step = static_cast<float>(img_dim - pat_dim) / (num_steps - 1);

            for (int j = 0; j < num_steps; ++j) 
            {
                int start_idx = std::round(actual_step * j);
                if (start_idx + pat_dim > img_dim) 
                {
                    start_idx = img_dim - pat_dim;
                }
                if (steps_here.empty() || start_idx != steps_here.back()) 
                {
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
inline Eigen::Tensor<float, 3, Eigen::ColMajor>
Inference::compute_gaussian(const std::array<int, 3> &patch_size, float sigma_scale,
                            float value_scaling_factor) 
{
    const int nX = patch_size[0];
    const int nY = patch_size[1];
    const int nZ = patch_size[2];

    auto get_gaussian_1d_tensor = [](int size, float sigma) 
    {
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

    if (max_val > 0.0f) 
    {
        float factor = value_scaling_factor / max_val;
        float min_val = factor * 1e-4f;
        gaussian_map = (gaussian_map * factor).cwiseMax(min_val);
    }
    return gaussian_map;
}
