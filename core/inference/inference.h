#pragma once

#include <memory>

#include <QString>

#ifdef _WIN32
    #define ENABLE_NPU_ADAPTER_ENUMERATION
    #include <dml_provider_factory.h>
#endif

#include <onnxruntime_cxx_api.h>#include <onnxruntime_cxx_api.h>

#include "utils/niftiVolume.h"

class InferencePrivate;

using OrtFloat16 = Ort::Float16_t;

class Inference 
{
  public:
    Inference();
    ~Inference();

    bool loadModel(const QString &modelPath);
    NiftiVolume run(const QString &modelPath, NiftiVolume &image,
                    const QString &destinationPath, const QString &inputName,
                    const QString &outputName);

  private:
    std::unique_ptr<InferencePrivate> d;

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
    static std::vector<std::vector<int>> sliceVolume(std::array<int, 3> image_size,
                                              std::array<int, 3> patch_size, float step_size);

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
    static Eigen::Tensor<float, 3, Eigen::ColMajor> compute_gaussian(const std::array<int, 3> &patch_size,
                                                              float sigma_scale = 0.125f,
                                                              float value_scaling_factor = 10.0f);
};