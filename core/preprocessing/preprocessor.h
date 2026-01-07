#pragma once

extern "C" {
#include <nifti1_io.h>
}
#include <zlib.h>

#include "resampling.h"
#include "brainextraction.h"
#include "preprocVolume.h"

#include <utils/animawrapper.h>

#include <iostream>
#include <vector>
#include <array>
#include <stdexcept>
#include <cassert>
#include <QStringList>

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
        AnimaWrapper wrapper;

        Volume4D loadVolume(const QString &path);
        void zScoreNormalize(Volume4D &vol, const Volume4D *seg = nullptr);

        std::vector<bool> computeNonZeroMask(const Volume4D &vol);

        Volume4D cropToNonZero(const Volume4D &data, Volume4D *seg, int nonzero_label,
                               std::array<std::array<int, 2>, 3> *bbox_out);

        std::pair<Volume4D, std::vector<std::array<int, 2>>> padVolume(const Volume4D &data,
                                                                       int min_size);

        QString reorientToRAS(const std::string &input_path, const std::string &prefix);

        QString biasCorrect(const std::string &input_path, const std::string &prefix);

        PreprocessedVolume preprocessModality(Preprocessor &pp, const std::string &modality_path,
                                              bool is_MNI, const std::array<int, 3> *bbox_ptr);

    };
} // namespace preprocessing

