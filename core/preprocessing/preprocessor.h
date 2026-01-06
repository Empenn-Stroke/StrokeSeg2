#pragma once

extern "C" {
#include <nifti1_io.h>
}
#include <zlib.h>

#include "resampling.h"
#include "brainextraction.h"
#include <iostream>
#include <vector>
#include <array>
#include <stdexcept>

namespace preprocessing {
    class Preprocessor {
      public:

        Preprocessor(Resampling res, BrainExtraction *br) : resampler(res), brainExtraction(br) {};

        ~Preprocessor() {};

        /**
         * @brief Performs preprocessing steps on the input volume, including brain extraction and
         * resampling.
         * @param vol(Volume4D): Input volume to preprocess.
         * @param target_spacing(std::array<float, 3>): Desired spacing for resampling (sx', sy',
         * sz').
         * @return (Volume4D): Preprocessed volume with brain extracted and resampled to target
         * spacing.
         */

        Volume4D preprocess(const Volume4D &vol, const std::array<float, 3> &target_spacing);

      private:
        Resampling resampler;
        BrainExtraction *brainExtraction;

        Volume4D loadVolume(const QString &path);
        void zScoreNormalize(Volume4D &vol, const Volume4D *seg = nullptr);


    };
} // namespace preprocessing

