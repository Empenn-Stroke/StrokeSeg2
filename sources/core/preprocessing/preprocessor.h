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
     * @brief Performs the required preprocessing on MRI data prior to the inference step.
     *
     * This class centralizes the main preprocessing steps applied to the input volumes before inference.
     * It is designed to be modular and extensible, allowing for easy integration of
     * additional steps or alternative algorithms as needed. The pipeline is optimized for typical
     * neuroimaging workflows, particularly in the context of stroke lesion segmentation, but can be
     * adapted for other applications with similar requirements.
     *
     * Utility :
     * - Bias correction to correct for intensity inhomogeneities.
     * - Registration to a reference atlas (MNI).
     * - Skull stripping.
     * - Statistical normalization (z-score).
     * - Cropping and padding to fit inference input dimension.
     */
    class Preprocessor : public QObject {
        Q_OBJECT
      public:
        /**
         * @brief Constructor for the Preprocessor class.
         * @param resampler Pointer to the Resampler instance.
         * @param brainExtractorT1 Pointer to the BrainExtractor instance for T1 modalities.
         * @param brainExtractorT2 Pointer to the BrainExtractor instance for T2 modalities.
         * @param animaWrapper Pointer to the AnimaWrapper instance for executing Anima commands.
         */
        Preprocessor(Resampler *resampler, BrainExtractor *brainExtractorT1, BrainExtractor *brainExtractorT2, AnimaWrapper *animaWrapper)
            : m_resampler(*resampler), m_brainExtractionT1(brainExtractorT1), m_brainExtractionT2(brainExtractorT2), m_wrapper(animaWrapper) {}

        /**
         * @brief Destructor for the Preprocessor class.
         */
        ~Preprocessor() = default;

        /**
         * @brief Preprocesses the input volumes.
         * @param inputPaths Map of modality names to their respective input paths.
         * @param modalities List of Modality objects representing the input modalities.
         * @param tempDir Temporary directory for intermediate files.
         * @param betOnly Indicates if only skull stripping should be performed.
         * @param mni Indicates if registration to MNI space should be performed.
         * @param brainExtraction Indicates if skull stripping should be performed.
         * @param saveInterSteps Indicates if intermediate results should be saved.
         * @return PreprocessedVolume object containing the preprocessed data.
         */
        PreprocessedVolume preprocess(const QMap<QString, QString> &inputPaths, const QList<Modality> &modalities, const QString &tempDir, bool betOnly, bool mni = false, bool brainExtraction = true, bool saveInterSteps = false);

        /**
         * @brief Performs z-score normalization on the given volume.
         * @param volume NiftiVolume to be normalized.
         * @param segmentation Optional segmentation volume for intensity estimation.
         * @return 0 on success, non-zero on failure.
         */
        static int zScoreNormalize(NiftiVolume &volume, const NiftiVolume *segmentation = nullptr);

        /**
         * @brief Performs min-max normalization on the given volume.
         * @param volume NiftiVolume to be normalized.
         * @param min Minimum value for normalization.
         * @param max Maximum value for normalization.
         * @return 0 on success, non-zero on failure.
         */
        static int minMaxNormalize(NiftiVolume &volume, float min = 0.0, float max = 255.0);

      private:
        Resampler m_resampler;
        BrainExtractor *m_brainExtractionT1;
        BrainExtractor *m_brainExtractionT2;
        AnimaWrapper *m_wrapper;

      private:
        /**
         * @brief Returns the appropriate BrainExtractor instance based on the reference family.
         * @param family ReferenceFamily enum value (T1 or T2).
         * @return Pointer to the BrainExtractor instance.
         */
        BrainExtractor *brainExtractorFor(ReferenceFamily family) const { return (family == ReferenceFamily::T2) ? m_brainExtractionT2 : m_brainExtractionT1; }

        /**
         * @brief Returns the atlas image path for the given reference family.
         * @param family ReferenceFamily enum value (T1 or T2).
         * @return Atlas image path as a QString.
         */
        QString atlasFor(ReferenceFamily family) const { return brainExtractorFor(family)->atlasImage(); }

        /**
         * @brief Builds a mask from the given data tensor.
         * @param data Input data tensor.
         * @return 3D tensor representing the mask.
         */
        Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> buildMask(const Eigen::Tensor<float, 4, Eigen::ColMajor> &data);

        /**
         * @brief Computes the bounding box for the given mask.
         * @param mask Input mask tensor.
         * @return 3D array representing the bounding box.
         */
        std::array<std::array<int, 2>, 3> computeBBox(const Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> &mask);

        /**
         * @brief Crops the given volume to the non-zero region.
         * @param volume Input NiftiVolume.
         * @param segmentation Optional segmentation volume for cropping.
         * @param nonZeroLabel Label for non-zero region.
         * @param bboxOut Optional output for the bounding box.
         * @return Pair of cropped NiftiVolume and bounding box.
         */
        std::pair<NiftiVolume, NiftiVolume> cropToNonZero(const NiftiVolume &volume, const NiftiVolume *segmentation = nullptr, int nonZeroLabel = 1, std::array<std::array<int, 2>, 3> *bboxOut = nullptr);

        /**
         * @brief Pads the given volume to the specified minimum size and divisibility.
         * @param volume Input NiftiVolume.
         * @param minSize Minimum size for padding.
         * @param div Divisibility requirement for padding.
         * @return Pair of padded NiftiVolume and padding information.
         */
        std::pair<NiftiVolume, std::vector<std::array<int, 2>>> padVolume(const NiftiVolume &volume, int minSize, int div);

        /**
         * @brief Performs bias correction on the given input path.
         * @param input_path Path to the input volume.
         * @param prefix Prefix for output files.
         * @return Path to the bias-corrected volume.
         */
        QString biasCorrect(const QString &input_path, const QString &prefix);

        /**
         * @brief Registers the given input to the reference path.
         * @param inputPath Path to the input volume.
         * @param refPath Path to the reference volume.
         * @param basePathPrefix Base path prefix for output files.
         * @param prefixLabel Prefix label for output files.
         * @return Pair of registered volume path and transformation path.
         */
        std::pair<QString, QString> registerToReference(const QString &inputPath, const QString &refPath, const QString &basePathPrefix, const QString &prefixLabel);

        /**
         * @brief Preprocesses a single modality.
         * @param modalityPath Path to the modality volume.
         * @param atlasPath Path to the atlas volume.
         * @param workDir Working directory for intermediate files.
         * @param isMNI Indicates if registration to MNI space should be performed.
         * @param bboxPtr Optional output for the bounding box.
         * @param saveInterSteps Indicates if intermediate results should be saved.
         * @return PreprocessedVolume object containing the preprocessed data.
         */
        PreprocessedVolume preprocessModality(const QString &modalityPath, const QString &atlasPath, const QString &workDir, bool isMNI, std::array<std::array<int, 2>, 3> *bboxPtr = nullptr, bool saveInterSteps = false);

        /**
         * @brief Moves the processed image to the output directory.
         * @param imgPath Path to the processed image.
         * @return Path to the moved image.
         */
        QString moveToOutput(const QString &imgPath);

        /**
         * @brief Checks if an abort has been requested.
         * @throws std::runtime_error if an abort has been requested.
         */
        static void checkAbort();
    };

} // namespace preprocessing
