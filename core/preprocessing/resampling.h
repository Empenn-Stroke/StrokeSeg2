#pragma once

#include <utils/volume4d.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#ifndef CORE_PREPROCESSING_RESAMPLING_H
#define CORE_PREPROCESSING_RESAMPLING_H

namespace preprocessing {

    /**
     * @class Resampling
     * @brief Handles spatial resampling of 4D volumes during preprocessing.
     */
    class Resampling {
      public:
        /**
         * @brief Constructor of the resampling class.
         */
        Resampling();

        /**
         * @brief Destructor of the resampling class.
         */
        ~Resampling();

        /**
         * @brief Resamples a 4D volume to a new spatial spacing.
         *
         * Computes the new volume shape according to the desired spacing and
         * resamples the data using interpolation. Depending on the spacing
         * anisotropy, resampling can be performed either fully in 3D or
         * separately along a specific axis.
         *
         * @param vol Input volume to resample.
         * @param new_spacing Desired spacing (sx', sy', sz').
         * @return Resampled volume with updated spacing and dimensions.
         */
        Volume4D resample(const Volume4D &vol, const std::array<float, 3> &new_spacing);

      private:
        float separate_z_anisotropy_threshold = 3.0f;
        bool is_seg = false; // true if segmentation
        int order = 1;       // linear interpolation
        int order_z = 0;     // interpolation along low-resolution axis

        /**
         * @brief Checks whether separate Z-axis resampling is required.
         *
         * Compares the ratio between the minimum and maximum spacing
         * to the anisotropy threshold.
         *
         * @param spacing Input volume spacing.
         * @return True if separate Z resampling is required.
         */
        bool get_do_separate_z(const std::array<float, 3> &spacing);

        /**
         * @brief Computes the new volume shape after resampling.
         *
         * Each dimension is scaled according to the ratio between the
         * original and target spacing.
         *
         * @param old_shape Original volume shape (X, Y, Z).
         * @param old_spacing Original spacing (sx, sy, sz).
         * @param new_spacing Desired spacing (sx', sy', sz').
         * @return New volume shape after resampling.
         */
        std::array<int, 3> reshape(const std::array<int, 3> &old_shape,
                                   const std::array<float, 3> &old_spacing,
                                   const std::array<float, 3> &new_spacing);

        /**
         * @brief Finds the axis with the lowest spatial resolution.
         *
         * @param spacing Volume spacing (sx, sy, sz).
         * @return Index of the axis with the largest spacing.
         */
        int get_single_lowres_axis(const std::array<float, 3> &spacing);

        /**
         * @brief Determines whether separate-axis resampling is needed.
         *
         * Selects whether resampling should be performed separately along
         * a specific axis based on spacing anisotropy or forced parameters.
         *
         * @param spacing Original spacing (sx, sy, sz).
         * @param new_spacing Desired spacing (sx', sy', sz').
         * @param force_separate_z Forces separate Z resampling.
         * @param has_force Indicates whether a force flag is provided.
         * @return Pair indicating whether separation is required and the axis index.
         */
        std::pair<bool, int> determine_separate_axis(const std::array<float, 3> &spacing,
                                                     const std::array<float, 3> &new_spacing,
                                                     bool force_separate_z = false,
                                                     bool has_force = false);

        /**
         * @brief Performs linear interpolation between two values.
         *
         * @param v0 Value at t = 0.
         * @param v1 Value at t = 1.
         * @param t Interpolation parameter.
         * @return Interpolated value.
         */
        float linear_interp(float v0, float v1, float t);

        /**
         * @brief Computes the nearest valid index from a floating-point coordinate.
         *
         * @param x Continuous coordinate.
         * @param max_val Maximum valid index (exclusive).
         * @return Nearest valid integer index.
         */
        int nearest_index(float x, int max_val);

        /**
         * @brief Performs trilinear interpolation on a 3D volume.
         *
         * @param vol Input volume.
         * @param c Channel index.
         * @param x Continuous x-coordinate in voxel space.
         * @param y Continuous y-coordinate in voxel space.
         * @param z Continuous z-coordinate in voxel space.
         * @return Interpolated value at the given position.
         */
        float trilinear_interp(const Volume4D &vol, int c, float x, float y, float z);
    };

} // namespace preprocessing

#endif // CORE_PREPROCESSING_RESAMPLING_H
