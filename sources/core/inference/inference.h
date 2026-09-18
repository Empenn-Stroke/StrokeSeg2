// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <memory>

#include <QString>

#include <onnxruntime_cxx_api.h>

#include "utils/niftiVolume.h"

class Model;

using OrtFloat16 = Ort::Float16_t;

/**
 * @brief A class for performing inference using a trained model.
 *
 * This class provides functionality to run inference on a NIfTI volume using a specified
 * ONNX model. It includes methods for slicing the volume into patches, computing Gaussian
 * kernels for patch weighting, and executing the inference process.
 */
class Inference 
{
  public:
    /**
     * @brief Default constructor for the Inference class.
     */
    Inference() = default;

    /**
     * @brief Destructor for the Inference class.
     */
    ~Inference() = default;

    /**
     * @brief Run inference on a NIfTI volume using a specified model.
     *
     * @param model A shared pointer to the model to be used for inference.
     * @param image The NIfTI volume to run inference on.
     * @param destinationPath The path where the output NIfTI volume will be saved.
     * @param inputName The name of the input tensor in the model.
     * @param outputName The name of the output tensor in the model.
     * @param patchSize An array of three integers specifying the dimensions of each patch to extract
     *                  from the image volume (e.g., {patch_width, patch_height, patch_depth}).
     *                  Default is {128, 128, 128}.
     * @param numClasses The number of classes for the segmentation task. Default is 2.
     * @return The resulting NIfTI volume after running inference.
     */
    NiftiVolume run(std::shared_ptr<Model> model,
                NiftiVolume &image,
                const QString &destinationPath, 
                const QString &inputName,
                const QString &outputName, 
                std::array<int, 3> patchSize = {128, 128, 128},
                int numClasses = 2
    );

  private:
    /**
     * @brief Slice a 3D image volume into overlapping patches based on the specified image size,
     * patch size, and step size.
     *
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
    static std::vector<std::vector<int>> sliceVolume(std::array<int, 3> image_size,
                                              std::array<int, 3> patch_size, float step_size);

    /**
     * @brief Compute a 3D Gaussian weighting function (kernel) based on the specified patch size,
     * sigma scale, and value scaling factor.
     *
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
    static Eigen::Tensor<float, 3, Eigen::ColMajor> compute_gaussian(const std::array<int, 3> &patch_size,
                                                              float sigma_scale = 0.125f,
                                                              float value_scaling_factor = 10.0f);
};
