#pragma once

#include <QString>
#include <QStringList>
#include <array>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "brainextraction.h"
#include "preprocvolume.h"
#include "resampling.h"

#include <utils/animawrapper.h>
#include <utils/niftiVolume.h>

#include <managers/configmanager.h>

namespace preprocessing {

    /**
     * @class Preprocessor
     * @brief Manage the complet pipeline for 4D volume preprocessing.
     *
     * This class centralise the main preprocessing steps applied to the input volumes before inference. 
     * It is designed to be modular and extensible, allowing for easy integration of
     * additional steps or alternative algorithms as needed. The pipeline is optimized for typical
     * neuroimaging workflows, particularly in the context of stroke lesion segmentation, but can be
     * adapted for other applications with similar requirements.
     *
     * Utility :
     * - Biais correction to correct for intensity inhomogeneities.
     * - Registration to a reference atlas (MNI).
     * - Skull stripping.
     * - Statistic normalization (z-score).
     * - Cropping and padding to fit inference input dimension.
     */
    class Preprocessor {

      public:
        /**
         * @brief Builder for the Preprocessor class.
         * @param res Pointer to instance of the Resampling class.
         * @param br Pointer to instance of the BrainExtraction class.
         * @param wr Pointer to instance of the AnimaWrapper class for executing Anima commands.
         * @param save Indicates if intermediary results should be saved for
         * debugging purposes.
         */
        Preprocessor(Resampling *res, BrainExtraction *br, AnimaWrapper *wr, bool save)
            : m_resampler(*res), m_brainExtraction(br), m_wrapper(wr), m_save_intermediary_steps(save) {}

        ~Preprocessor() = default;

        PreprocessedVolume preprocess(const QString &t1_path, const QString &flair_path,
                                      const QString &temp_dir, bool bet_only);

      private:
        Resampling m_resampler;
        BrainExtraction *m_brainExtraction;
        AnimaWrapper *m_wrapper;
        bool m_save_intermediary_steps;
        ConfigManager &m_config = ConfigManager::instance();
        QString m_atlasImage = Paths::atlasDir() + "/Reference_T1.nrrd";

      private:
        
        void zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg = nullptr);

        Eigen::Tensor<uint8_t, 3, Eigen::ColMajor>
        buildMask(const Eigen::Tensor<float, 4, Eigen::ColMajor> &data);

        std::array<std::array<int, 2>, 3>
        computeBBox(const Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> &mask);

        std::pair<NiftiVolume, NiftiVolume>
        cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg = nullptr,
                      int nonzero_label = 1, std::array<std::array<int, 2>, 3> *bbox_out = nullptr);

        std::pair<NiftiVolume, std::vector<std::array<int, 2>>> padVolume(const NiftiVolume &vol,
                                                                          int min_size, int div);

        QString biasCorrect(const QString &input_path, const QString &prefix);

        
        std::pair<QString, QString> registerToReference(const QString &input_path,
                                                        const QString &mni_image_path,
                                                        const QString &prefix_label,
                                                        const QString &base_path_prefix);

        PreprocessedVolume
        preprocessModality(const QString &modality_path, bool is_MNI,
                           std::array<std::array<int, 2>, 3> *bbox_ptr = nullptr);

        static void printAction(const QString &actionName);

        QString moveToOutput(const QString &img_path);
    };

} // namespace preprocessing