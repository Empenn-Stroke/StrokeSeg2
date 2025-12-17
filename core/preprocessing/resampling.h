#pragma once

#include <utils/volume4d.h>
#include <array>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#ifndef PREPROCESSING_RESAMPLING_H
#define PREPROCESSING_RESAMPLING_H

namespace preprocessing {
    class Resampling {
    public:

        Resampling();
        ~Resampling();

        Volume4D resample(const Volume4D &vol, const std::array<float, 3> &new_spacing);

    private:

        float separate_z_anisotropy_threshold = 3.0f;
        bool is_seg = false; // true si segmentation
        int order = 1;       // interpolation linéaire
        int order_z = 0;     // interpolation le long de l'axe faible résolution



        /** 
        
        Compares ratio between the min and max spacing of the volume to the anisotropy threshold.
        
        Args:
            : Input volume data.

        Returns:
            bool: True if separate z resampling is needed, False otherwise.
        
        **/

        bool get_do_separate_z(const std::array<float, 3> & spacing);


        /**
        * Computes the new shape of data after resampling to a new spacing
        Scales each dimension according to the ratio of old and new spacing values

        * Args:
            old_shape (std::array<int, 3>): Original shape of the volume (X, Y, Z).
            old_spacing (std::array<float, 3>): Original spacing of the volume (sx, sy, sz).
            new_spacing (std::array<float, 3>): Desired new spacing of the volume (sx', sy', sz').

        * Returns:
            std::array<int, 3>: new shape of the volume after resampling (X', Y', Z').
        **/
        std::array<int, 3> reshape(const std::array<int, 3> &old_shape,
                                   const std::array<float, 3> &old_spacing,
                                   const std::array<float, 3> &new_spacing);


        /**
        * Finds which axis has the biggest spacing (=the lowest resolution)

        * Args:
            spacing (std::array<float, 3>): Spacing of the volume (sx, sy, sz).

        * Returns:
            int: Index of the axis with the lowest resolution.
        **/
        int get_single_lowres_axis(const std::array<float, 3> &spacing);


        /**
        
        * Determines whether separate axis resampling is needed and which axis to use
        * 
        * Args:
            spacing (std::array<float, 3>): Original spacing of the volume (sx, sy, sz).
            new_spacing (std::array<float, 3>): Desired new spacing of the volume (sx', sy', sz').
            force_separate_z (bool): Whether to force separate z resampling.
            has_force (bool): Whether a force flag is provided.
        * Returns:
            std::pair<bool, int>: A pair where the first element indicates if separate axis resampling is needed, 
                                    and the second element is the index of the axis to use for separate resampling.


        **/
        std::pair<bool, int> determine_separate_axis(const std::array<float, 3> &spacing,
                                                     const std::array<float, 3> &new_spacing,
                                                     bool force_separate_z=false, 
                                                     bool has_force=false
        );

        float trilinear_interp(const Volume4D &vol, int c, float x, float y, float z);
        int nearest_index(float x, int max_val);
        float linear_interp(float v0, float v1, float t);

    };
}


#endif // PREPROCESSING_RESAMPLING_H