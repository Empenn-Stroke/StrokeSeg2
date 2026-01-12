#pragma once

#include <QString>
#include <QStringList>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "brainextraction.h"
#include "preprocVolume.h"
#include "resampling.h"
#include <utils/animawrapper.h>
#include <utils/niftiVolume.h>

namespace preprocessing {

    /**
     * @class Preprocessor
     * @brief Handles preprocessing of 4D volumes (brain extraction, normalization, resampling,
     * cropping, padding, bias correction)
     *
     * This class uses Resampling and BrainExtraction objects and operates
     * on NiftiVolume objects with Eigen tensors.
     */
    class Preprocessor {
      public:
        Preprocessor(Resampling res, BrainExtraction *br) : resampler(res), brainExtraction(br) {}

        ~Preprocessor() = default;

        /**
         * @brief Full preprocessing pipeline for a 4D volume.
         * @param vol Input NiftiVolume.
         * @param target_spacing Desired spacing in mm (sx', sy', sz').
         * @param is_segmentation True if the volume is a segmentation.
         * @return Preprocessed NiftiVolume (brain extracted, normalized, resampled).
         */
        NiftiVolume preprocess(const NiftiVolume &vol, const Eigen::Vector3f &target_spacing,
                               bool is_segmentation = false);

      private:
        Resampling resampler;
        BrainExtraction *brainExtraction;
        AnimaWrapper wrapper;

        void zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg = nullptr);

        std::vector<bool> computeNonZeroMask(const NiftiVolume &vol);
        NiftiVolume cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg = nullptr,
                                  int nonzero_label = 1,
                                  std::array<std::array<int, 2>, 3> *bbox_out = nullptr);
        std::pair<NiftiVolume, std::vector<std::array<int, 2>>> padVolume(const NiftiVolume &vol,
                                                                          int min_size);

        QString reorientToRAS(const std::string &input_path, const std::string &prefix);
        QString biasCorrect(const std::string &input_path, const std::string &prefix);

        PreprocessedVolume preprocessModality(Preprocessor &pp, const std::string &modality_path,
                                              bool is_MNI,
                                              const std::array<int, 3> *bbox_ptr = nullptr);
    };

} // namespace preprocessing