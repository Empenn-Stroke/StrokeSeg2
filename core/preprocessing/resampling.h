#pragma once

#include <Eigen/Core>
#include <set>
#include <unsupported/Eigen/CXX11/Tensor>
#include <utility>
#include <vector>
#include <utils/niftiVolume.h>


#ifndef CORE_PREPROCESSING_RESAMPLING_H
#define CORE_PREPROCESSING_RESAMPLING_H


namespace preprocessing 
{

    /**
     * @class Resampling
     * @brief Handles spatial resampling of 4D volumes using NiftiVolume and Eigen.
     *
     * This class provides functionality to resample 4D volumes (C x X x Y x Z)
     * to a new spacing, optionally handling anisotropic axes separately.
     *
     * Features:
     * - Linear interpolation for images.
     * - Nearest / vote interpolation for segmentations.
     * - Slice-by-slice resampling along the axis with the lowest resolution.
     * - Fully compatible with Eigen::Tensor<float,4,Eigen::RowMajor> via NiftiVolume.
     */
    class Resampling 
    {
      public:
        /**
         * @brief Default constructor.
         */
        Resampling() = default;

        NiftiVolume resample(const NiftiVolume &in, const Eigen::Vector3f &new_spacing,
                             bool is_segmentation = false);

      private:
        /// Threshold to detect if an axis is low-resolution and should be resampled separately
        float separate_z_anisotropy_threshold = 3.0f;

        bool get_do_separate_axis(const Eigen::Vector3f &spacing) const;
        
        int get_lowres_axis(const Eigen::Vector3f &spacing) const;

        std::pair<bool, int> determine_separate_axis(const Eigen::Vector3f &current_spacing,
                                                     const Eigen::Vector3f &new_spacing) const;

        Eigen::Vector3i compute_new_shape(const Eigen::Vector3i &old_shape,
                                          const Eigen::Vector3f &old_spacing,
                                          const Eigen::Vector3f &new_spacing) const;

        float bilinear_2d(const NiftiVolume &vol, int c, float x, float y, int slice,
                          int axis) const;

        float nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i, int j,
                         float pos, int axis) const;

        float vote_nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i,
                              int j, float pos, int axis) const;

        Eigen::ArrayXi generate_indices(int new_dim, float ratio, int max_val);
    };

} // namespace preprocessing


#endif // CORE_PREPROCESSING_RESAMPLING_H


