#pragma once

#include <QString>
#include <QStringList>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "brainextraction.h"
#include "preprocvolume.h"
#include "resampling.h"

#include <utils/animawrapper.h>
#include <utils/niftiVolume.h>

#include <managers/configmanager.h>

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
        Preprocessor(Resampling *res, BrainExtraction *br, AnimaWrapper *wr) : resampler(*res), brainExtraction(br), wrapper(wr) {}

        ~Preprocessor() = default;

        PreprocessedVolume preprocess(const QString &t1_path, const QString &flair_path,
                                      const QString &temp_dir, bool bet_only);

      private:
        std::vector<QString> preprocessing_steps;
        Resampling resampler;
        BrainExtraction *brainExtraction;
        AnimaWrapper *wrapper;
        ConfigManager &config = ConfigManager::instance();
        QString atlasImage = atlas_dir + "/Reference_T1.nrrd";

        void 
        zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg = nullptr);

        std::vector<bool> 
        computeNonZeroMask(const NiftiVolume &vol);

        std::pair<NiftiVolume, NiftiVolume>
        cropToNonZero(const NiftiVolume &vol, 
                                  const NiftiVolume *seg = nullptr,
                                  int nonzero_label = 1,
                                  std::array<std::array<int, 2>, 3> *bbox_out = nullptr);

        std::pair<NiftiVolume, std::vector<std::array<int, 2>>> 
        padVolume(const NiftiVolume &vol, int min_size, int div);

        QString 
        reorientToRAS(const QString &input_path, const QString &prefix);

        QString 
        biasCorrect(const QString &input_path, const QString &prefix);

        std::pair<QString, QString> 
        registerToReference(const QString &input_path,
                            const QString &mni_image_path,
                            const QString &prefix_label,
                            const QString &base_path_prefix);

        PreprocessedVolume preprocessModality(const QString &modality_path,
                                              bool is_MNI,
                                              std::array<std::array<int, 2>, 3> *bbox_ptr = nullptr);

        static void printAction(const QString &actionName);

        QString moveToOutput(const QString &img_path);
    };

} // namespace preprocessing