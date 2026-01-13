#pragma once

#include <Eigen/Core>
#include <set>
#include <unsupported/Eigen/CXX11/Tensor>
#include <utility>
#include <vector>
#include <utils/niftiVolume.h>


#ifndef CORE_PREPROCESSING_RESAMPLING_H
#define CORE_PREPROCESSING_RESAMPLING_H


namespace preprocessing {

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
    class Resampling {
      public:
        /**
         * @brief Default constructor.
         */
        Resampling() = default;

        /**
         * @brief Resample a 4D volume to a new spacing.
         *
         * This function will perform full 3D linear interpolation if the spacing
         * is isotropic, or slice-by-slice resampling along the axis with the
         * largest spacing if anisotropy is detected.
         *
         * @param in Input NiftiVolume.
         * @param new_spacing Desired spacing in mm (sx', sy', sz').
         * @param is_segmentation True if the volume is a segmentation, using nearest/vote
         * interpolation.
         * @return Resampled NiftiVolume.
         */
        NiftiVolume resample(const NiftiVolume &in, const Eigen::Vector3f &new_spacing,
                             bool is_segmentation = false);

      private:
        /// Threshold to detect if an axis is low-resolution and should be resampled separately
        float separate_z_anisotropy_threshold = 3.0f;

       /**
         * @brief Checks if any axis requires separate resampling due to anisotropy.
         * @param spacing Spacing vector (sx, sy, sz).
         * @return True if separate axis resampling is needed.
         */
        bool get_do_separate_axis(const Eigen::Vector3f &spacing) const;
        /**
         * @brief Finds the axis with the largest spacing (lowest resolution).
         * @param spacing Spacing vector (sx, sy, sz).
         * @return Index of the axis with largest spacing, or -1 if ambiguous.
         */
        int get_lowres_axis(const Eigen::Vector3f &spacing) const;

        /**
         * @brief Determines if resampling should be done separately along a single axis.
         *
         * @param current_spacing Current volume spacing.
         * @param new_spacing Target spacing.
         * @return Pair: {true if separate resampling needed, axis index}.
         */

        std::pair<bool, int> determine_separate_axis(const Eigen::Vector3f &current_spacing,
                                                     const Eigen::Vector3f &new_spacing) const;

        /**
         * @brief Computes new shape after resampling.
         *
         * Each dimension is scaled according to the ratio between original and target spacing.
         *
         * @param old_shape Original shape (X, Y, Z).
         * @param old_spacing Original spacing (sx, sy, sz).
         * @param new_spacing Target spacing (sx', sy', sz').
         * @return New shape (X', Y', Z').
         */
        Eigen::Vector3i compute_new_shape(const Eigen::Vector3i &old_shape,
                                          const Eigen::Vector3f &old_spacing,
                                          const Eigen::Vector3f &new_spacing) const;

        /**
         * @brief Performs bilinear interpolation on a 2D slice of a 3D volume.
         *
         * @param vol Input volume.
         * @param c Channel index.
         * @param x Continuous x-coordinate.
         * @param y Continuous y-coordinate.
         * @param slice Slice index along the low-res axis.
         * @param axis Axis of low resolution (0=X,1=Y,2=Z).
         * @return Interpolated value.
         */
        float bilinear_2d(const NiftiVolume &vol, int c, float x, float y, int slice,
                          int axis) const;

        /**
         * @brief Nearest-neighbor interpolation along 1D axis.
         *
         * @param t Tensor data.
         * @param c Channel index.
         * @param i Coordinate along first orthogonal axis.
         * @param j Coordinate along second orthogonal axis.
         * @param pos Continuous position along axis.
         * @param axis Axis index to interpolate along.
         * @return Interpolated value.
         */
        float nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i, int j,
                         float pos, int axis) const;

        /**
         * @brief Nearest / vote interpolation for segmentation along 1D axis.
         *
         * @param t Tensor data.
         * @param c Channel index.
         * @param i Coordinate along first orthogonal axis.
         * @param j Coordinate along second orthogonal axis.
         * @param pos Continuous position along axis.
         * @param axis Axis index to interpolate along.
         * @return Interpolated value (nearest label).
         */
        float vote_nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i,
                              int j, float pos, int axis) const;
    };

} // namespace preprocessing


#endif // CORE_PREPROCESSING_RESAMPLING_H


