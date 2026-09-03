// SPDX-License-Identifier: AGPL-3.0-or-later

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

#include <managers/configmanager.h>
#include <utils/animawrapper.h>
#include <utils/niftiVolume.h>
#include <utils/modality.h>

namespace preprocessing {

    /**
     * @class Preprocessor
     * @brief Performs the required preprocessing on MRI data prior to the inference step..
     *
     * This class centralizes the main preprocessing steps applied to the input volumes before inference.
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
    class Preprocessor : public QObject {
        Q_OBJECT
      public:
        /**
         * @brief Builder for the Preprocessor class.
         * @param res Pointer to instance of the Resampling class.
         * @param br Pointer to instance of the BrainExtraction class.
         * @param wr Pointer to instance of the AnimaWrapper class for executing Anima commands.
         * @param save Indicates if intermediary results should be saved for
         * debugging purposes.
         */
        Preprocessor(Resampler *resampler, BrainExtractor *brainExtractorT1, BrainExtractor *brainExtractorT2, AnimaWrapper *animaWrapper)
            : m_resampler(*resampler), m_brainExtractionT1(brainExtractorT1), m_brainExtractionT2(brainExtractorT2), m_wrapper(animaWrapper) {}

        ~Preprocessor() = default;


        PreprocessedVolume preprocess(const QMap<QString, QString> &inputPaths, const QList<Modality> &modalities, const QString &tempDir, bool betOnly, bool mni = false, bool brainExtraction = true, bool saveInterSteps = false);

        static int zScoreNormalize(NiftiVolume &volume, const NiftiVolume *segmentation = nullptr);

        static int minMaxNormalize(NiftiVolume &volume, float min = 0.0, float max = 255.0);

      private:
        Resampler m_resampler;
        BrainExtractor *m_brainExtractionT1;
        BrainExtractor *m_brainExtractionT2;
        AnimaWrapper *m_wrapper;

      private:
        BrainExtractor *brainExtractorFor(ReferenceFamily family) const { return (family == ReferenceFamily::T2) ? m_brainExtractionT2 : m_brainExtractionT1; }

        QString atlasFor(ReferenceFamily family) const { return brainExtractorFor(family)->atlasImage(); }

        Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> buildMask(const Eigen::Tensor<float, 4, Eigen::ColMajor> &data);
        std::array<std::array<int, 2>, 3> computeBBox(const Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> &mask);
        std::pair<NiftiVolume, NiftiVolume> cropToNonZero(const NiftiVolume &volume, const NiftiVolume *segmentation = nullptr, int nonZeroLabel = 1, std::array<std::array<int, 2>, 3> *bboxOut = nullptr);
        std::pair<NiftiVolume, std::vector<std::array<int, 2>>> padVolume(const NiftiVolume &volume, int minSize, int div);
        QString biasCorrect(const QString &input_path, const QString &prefix);
        std::pair<QString, QString> registerToReference(const QString &inputPath, const QString &refPath, const QString &basePathPrefix, const QString &prefixLabel);
        PreprocessedVolume preprocessModality(const QString &modalityPath, const QString &atlasPath, const QString &workDir, bool isMNI, std::array<std::array<int, 2>, 3> *bboxPtr = nullptr, bool saveInterSteps = false);


        QString moveToOutput(const QString &imgPath);

        static void checkAbort();
    };

} // namespace preprocessing
