// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <Eigen/Core>
#include <set>
#include <unsupported/Eigen/CXX11/Tensor>
#include <utility>
#include <vector>
#include <utils/niftiVolume.h>

namespace preprocessing {

    /**
     * @class Resampler
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
    class Resampler
    {
    public:
        /**
         * @brief Default constructor.
         */
        Resampler() = default;

        /**
         * @brief Resample the input NiftiVolume to a new spacing.
         *
         * @param in Input NiftiVolume.
         * @param new_spacing Target spacing for the resampled volume.
         * @param is_segmentation Indicates whether the volume is a segmentation.
         * @return Resampled NiftiVolume.
         */
        NiftiVolume resample(const NiftiVolume &in, const Eigen::Vector3f &new_spacing, bool is_segmentation = false);

      private:
        /**
         * @brief Determine if a separate axis should be resampled.
         *
         * @param spacing Current spacing of the volume.
         * @return True if a separate axis should be resampled, false otherwise.
         */
        bool get_do_separate_axis(const Eigen::Vector3f &spacing) const;
        
        /**
         * @brief Get the axis with the lowest resolution.
         *
         * @param spacing Current spacing of the volume.
         * @return Index of the axis with the lowest resolution.
         */
        int get_lowres_axis(const Eigen::Vector3f &spacing) const;

        /**
         * @brief Determine if a separate axis should be resampled based on current and new spacing.
         *
         * @param current_spacing Current spacing of the volume.
         * @param new_spacing Target spacing for the resampled volume.
         * @return Pair indicating if a separate axis should be resampled and the axis index.
         */
        std::pair<bool, int> determine_separate_axis(const Eigen::Vector3f &current_spacing,
                                                     const Eigen::Vector3f &new_spacing) const;

        /**
         * @brief Compute the new shape of the resampled volume.
         *
         * @param old_shape Original shape of the volume.
         * @param old_spacing Original spacing of the volume.
         * @param new_spacing Target spacing for the resampled volume.
         * @return New shape of the resampled volume.
         */
        Eigen::Vector3i compute_new_shape(const Eigen::Vector3i &old_shape,
                                          const Eigen::Vector3f &old_spacing,
                                          const Eigen::Vector3f &new_spacing) const;

        /**
         * @brief Perform 2D bilinear interpolation.
         *
         * @param vol Input NiftiVolume.
         * @param c Channel index.
         * @param x X-coordinate for interpolation.
         * @param y Y-coordinate for interpolation.
         * @param slice Slice index.
         * @param axis Axis along which to interpolate.
         * @return Interpolated value.
         */
        float bilinear_2d(const NiftiVolume &vol, int c, float x, float y, int slice, int axis) const;

        /**
         * @brief Perform 1D nearest neighbor interpolation.
         *
         * @param t Input tensor.
         * @param c Channel index.
         * @param i X-index.
         * @param j Y-index.
         * @param pos Position for interpolation.
         * @param axis Axis along which to interpolate.
         * @return Interpolated value.
         */
        float nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i, int j, float pos, int axis) const;

        /**
         * @brief Perform 1D vote nearest neighbor interpolation.
         *
         * @param t Input tensor.
         * @param c Channel index.
         * @param i X-index.
         * @param j Y-index.
         * @param pos Position for interpolation.
         * @param axis Axis along which to interpolate.
         * @return Interpolated value.
         */
        float vote_nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i, int j, float pos, int axis) const;

        /**
         * @brief Generate indices for resampling.
         *
         * @param new_dim New dimension size.
         * @param ratio Ratio for interpolation.
         * @param max_val Maximum value for indexing.
         * @return Array of indices.
         */
        Eigen::ArrayXi generate_indices(int new_dim, float ratio, int max_val);

    private:
        float separate_z_anisotropy_threshold = 3.0f; /// Threshold to detect if an axis is low-resolution and should be resampled separately
    };

} // namespace preprocessing
