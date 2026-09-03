// SPDX-License-Identifier: AGPL-3.0-or-later

#include "preprocessor.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <nifti1_io.h>

#include <QtDebug>
#include <qelapsedtimer.h>

#include "managers/progressManager.h"
#include "utils/log.h"

namespace preprocessing
{

    Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> Preprocessor::buildMask(const Eigen::Tensor<float, 4, Eigen::ColMajor> &data) 
    {
        const int dimX = (int)data.dimension(0);
        const int dimY = (int)data.dimension(1);
        const int dimZ = (int)data.dimension(2);

        Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> mask(dimX, dimY, dimZ);
        mask.setZero();

        for (int c = 0; c < data.dimension(3); ++c) 
        {
            mask = mask + (data.chip(c, 3) != 0.0f).cast<uint8_t>();
        }

        return (mask > (uint8_t)0).cast<uint8_t>();
    }

    int Preprocessor::minMaxNormalize(NiftiVolume &volume, float min, float max)
    {
        int res = 0;

        auto &tensor = volume.data;
        float *dataPtr = tensor.data();
        int totalElements = (int)tensor.size();

        float minVal = std::numeric_limits<float>::max();
        float maxVal = std::numeric_limits<float>::lowest();
        bool hasBrainVoxels = false;

        for (int i = 0; i < totalElements; ++i)
        {
            if (dataPtr[i] != 0.0f) 
            {
                if (dataPtr[i] < minVal)
                {
                    minVal = dataPtr[i];
                }
                if (dataPtr[i] > maxVal) 
                {
                    maxVal = dataPtr[i];
                }
                hasBrainVoxels = true;
            }
        }

        if (!hasBrainVoxels || (maxVal - minVal) < 1e-5f)
        {
            res = -1;
        } 
        else 
        {
            float range = maxVal - minVal;
            float targetRange = max - min;
            float scale = targetRange / range;

            for (int i = 0; i < totalElements; ++i) 
            {
                if (dataPtr[i] != 0.0f)
                {
                    dataPtr[i] = (dataPtr[i] - minVal) * scale + min;
                }
            }

            res = 0;
        }

        return res;
    }

    int Preprocessor::zScoreNormalize(NiftiVolume &volume, const NiftiVolume *segmentation) 
    {
        int res = 0;

        auto &tensor = volume.data;

        double sum = 0.0;
        int count = 0;
        float *dataPtr = tensor.data();
        int totalElements = (int)tensor.size();

        for (int i = 0; i < totalElements; ++i)
        {
            if (dataPtr[i] != 0.0f) 
            {
                sum += dataPtr[i];
                count++;
            }
        }

        if (count == 0) 
        {
            res = -1;
        } 
        else 
        {
            double mean = sum / count;

            double squareSum = 0.0;
            for (int i = 0; i < totalElements; ++i)
            {
                if (dataPtr[i] != 0.0f)
                {
                    double diff = dataPtr[i] - mean;
                    squareSum += diff * diff;
                }
            }
            float standardDev = static_cast<float>(std::sqrt(std::max(squareSum / count, 1e-8)));
            float fMean = static_cast<float>(mean);

            for (int i = 0; i < totalElements; ++i)
            {
                if (dataPtr[i] != 0.0f) 
                {
                    dataPtr[i] = (dataPtr[i] - fMean) / standardDev;
                }
            }

            res = 0;
        }

        return res;
    }

    std::pair<NiftiVolume, std::vector<std::array<int, 2>>> Preprocessor::padVolume(const NiftiVolume &volume, int minSize, int div) 
    {
        const int dimX = (int)volume.data.dimension(0);
        const int dimY = (int)volume.data.dimension(1);
        const int dimZ = (int)volume.data.dimension(2);
        const int dimC = (int)volume.data.dimension(3);

        auto getTargetSize = [minSize, div](int current) 
        {
            int base = std::max(current, minSize);
            return (base + div - 1) / div * div;
        };

        int targetX = getTargetSize(dimX);
        int targetY = getTargetSize(dimY);
        int targetZ = getTargetSize(dimZ);

        int padX = (targetX - dimX) / 2;
        int padY = (targetY - dimY) / 2;
        int padZ = (targetZ - dimZ) / 2;

        NiftiVolume padded;
        padded.spacing = volume.spacing;
        padded.data.resize(targetX, targetY, targetZ, dimC);
        padded.data.setConstant(0.0f);

        Eigen::array<Eigen::Index, 4> offsets = {padX, padY, padZ, 0};
        Eigen::array<Eigen::Index, 4> extents = {dimX, dimY, dimZ, dimC};
        padded.data.slice(offsets, extents) = volume.data;

        std::vector<std::array<int, 2>> p_info = {{padX, dimX + padX}, {padY, dimY + padY}, {padZ, dimZ + padZ}};

        return {padded, p_info};
    }

    std::array<std::array<int, 2>, 3> Preprocessor::computeBBox(const Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> &mask) 
    {
        auto xAny = mask.any(Eigen::array<int, 2>{1, 2});
        auto yAny = mask.any(Eigen::array<int, 2>{0, 2});
        auto zAny = mask.any(Eigen::array<int, 2>{0, 1});

        auto getRange = [](const Eigen::Tensor<bool, 1, Eigen::ColMajor> &anyTensor, int size) -> std::array<int, 2> 
        {
            std::array<int, 2> result = {0, 0};
            int minIdx = 0;
            int maxIdx = size - 1;
            while (minIdx < size && !anyTensor(minIdx))
            {
                minIdx++;
            }
            while (maxIdx >= 0 && !anyTensor(maxIdx)) 
            {
                maxIdx--;
            }
            if (!(minIdx > maxIdx)) 
            {
                result = {minIdx, maxIdx + 1};
            }

            return result;
        };

        auto rangeX = getRange(xAny, (int)mask.dimension(0));
        auto rangeY = getRange(yAny, (int)mask.dimension(1));
        auto rangeZ = getRange(zAny, (int)mask.dimension(2));

        if (rangeX[1] == 0)
        {
            throw std::runtime_error("Empty mask");
        }
        return {rangeX, rangeY, rangeZ};
    }

    std::pair<NiftiVolume, NiftiVolume> Preprocessor::cropToNonZero(const NiftiVolume &volume, const NiftiVolume *segmentation, int nonZeroLabel, std::array<std::array<int, 2>, 3> *bboxOut)
    {
        auto mask = buildMask(volume.data);
        auto bbox = computeBBox(mask);
        if (bboxOut)
        {
            *bboxOut = bbox;
        }

        int x0 = bbox[0][0], newX = bbox[0][1] - x0;
        int y0 = bbox[1][0], newY = bbox[1][1] - y0;
        int z0 = bbox[2][0], newZ = bbox[2][1] - z0;
        int C = (int)volume.data.dimension(3);

        NiftiVolume cropped;
        cropped.spacing = volume.spacing;

        Eigen::array<Eigen::Index, 4> offsets = {x0, y0, z0, 0};
        Eigen::array<Eigen::Index, 4> extents = {newX, newY, newZ, C};

        cropped.data = volume.data.slice(offsets, extents);

        return {cropped, cropped};
    }

    QString Preprocessor::biasCorrect(const QString &inputPath, const QString &prefix) 
    {
        QString outDir = QFileInfo(prefix).absolutePath();
        QString cleanPrefix = QFileInfo(prefix).fileName();
        QString outputPath = outDir + "/" + cleanPrefix + "_N4.nii.gz";

        QStringList args;
        args << "animaN4BiasCorrection" << "-i" << inputPath << "-o" << outputPath;

        int ret = m_wrapper->run(args);
        if (ret != 0) 
        {
            std::string errMsg = m_wrapper->lastStderr().toStdString();
            if (errMsg.empty())
            {
                errMsg = "Unknown error in AnimaWrapper";
            }
            throw std::runtime_error("Bias correction failed: " + errMsg);
        }

        return outputPath;
    }

    std::pair<QString, QString> Preprocessor::registerToReference(const QString &inputPath, const QString &refPath, const QString &basePathPrefix, const QString &prefixLabel) 
    {
        QString cleanSuffix = QFileInfo(basePathPrefix).fileName();

        QString outDir = QFileInfo(basePathPrefix).absolutePath();
        const QString outputPath = outDir + "/" + prefixLabel + "_" + cleanSuffix + ".nii.gz";

        QString trsfBase = outputPath;
        if (trsfBase.endsWith(".nii.gz"))
        {
            trsfBase.chop(7);
        }
        const QString trsfPath = trsfBase + ".txt";

        QStringList args;
        args << "animaPyramidalBMRegistration"
             << "-r" << refPath << "-m" << inputPath << "-o" << outputPath << "-O" << trsfPath;

        int ret = m_wrapper->run(args);
        if (ret != 0) 
        {
            std::string errMsg = m_wrapper->lastStderr().toStdString();
            if (errMsg.empty()) 
            {
                errMsg = "Unknown error in AnimaWrapper";
            }
            throw std::runtime_error("Bias correction failed: " + errMsg);
        }

        return std::pair<QString, QString>(outputPath, trsfPath);
    }

    /**
     * @brief Performs the complete preprocessing pipeline on a single modality, including bias
     * correction, registration to MNI space, cropping, resampling, normalization, and padding.
     * @param modalityPath Path to the input image modality.
     * @param atlasPath Path to the MNI atlas to register this modality against (T1 or T2 family).
     * @param isMNI Defines if the input image is already in MNI space, in which case bias
     * correction and registration are skipped.
     * @param bboxPtr Coordinates of the bounding box used for cropping. If provided, it will be
     * filled with the coordinates of the cropping box. default is nullptr.
     * @return PreprocessedVolume for this modality.
     */
    PreprocessedVolume
    Preprocessor::preprocessModality(const QString &modalityPath, const QString &atlasPath, const QString &workDir, bool isMNI, std::array<std::array<int, 2>, 3> *bboxPtr, bool saveInterSteps) 
    {
        PreprocessedVolume result;
        QString path = modalityPath;

        result.originalPath = modalityPath;

        qDebug() << "Preprocessing modality:" << modalityPath;

        QString debugPrefix = workDir + "/" + QFileInfo(modalityPath).baseName();   // <-- workDir, plus le dossier de modalityPath

        if (!isMNI) 
        {
            ProgressManager::instance().report(41, 9, 10, new QString("Registering to MNI space"));

            printAction("bias correction");
            QString prefix = workDir + "/" + QFileInfo(path).baseName();   // <-- idem
            path = biasCorrect(path, prefix);

            printAction("registration to MNI atlas");
            auto [regPath, trsf] = registerToReference(path, atlasPath, prefix, "MNI");
            path = regPath;
            result.trsfPath = trsf;

            qDebug() << "trsf path:" << trsf;
        }

        ProgressManager::instance().report(41, 9, 20, new QString("Cropping to non-zero content"));

        printAction("loading NIFTI volume");
        NiftiVolume volume = NiftiVolume::loadNifti(path);

        if (saveInterSteps)
        {
            NiftiVolume::saveNifti(debugPrefix + "_loaded.nii.gz", volume);
        }

        result.originalShape = Eigen::Vector3i((int)volume.data.dimension(0), (int)volume.data.dimension(1), (int)volume.data.dimension(2));

        printAction("cropping to non-zero content");
        std::array<std::array<int, 2>, 3> localBbox = {{{-1, -1}, {-1, -1}, {-1, -1}}};
        auto [cropped, _] = cropToNonZero(volume, nullptr, -1, bboxPtr ? bboxPtr : &localBbox);

        ProgressManager::instance().report(41, 9, 40);

        if (bboxPtr && (*bboxPtr)[0][0] == -1) 
        {
            *bboxPtr = localBbox;
        }

        auto &bboxRef = bboxPtr ? *bboxPtr : localBbox;
        qInfo().noquote() << QString("[BBOX] X: [%1, %2], Y: [%3, %4], Z: [%5, %6] (Size: %7x%8x%9)")
                                 .arg(bboxRef[0][0])
                                 .arg(bboxRef[0][1])
                                 .arg(bboxRef[1][0])
                                 .arg(bboxRef[1][1])
                                 .arg(bboxRef[2][0])
                                 .arg(bboxRef[2][1])
                                 .arg(bboxRef[0][1] - bboxRef[0][0])
                                 .arg(bboxRef[1][1] - bboxRef[1][0])
                                 .arg(bboxRef[2][1] - bboxRef[2][0]);

        result.bbox = localBbox;

        if (saveInterSteps)
        {
            NiftiVolume::saveNifti(debugPrefix + "_cropped.nii.gz", cropped);
        }

        ProgressManager::instance().report(41, 9, 80, new QString("Resampling to 1.0mm iso"));

        printAction("resampling to 1.0mm iso");
        NiftiVolume resampledVolume = m_resampler.resample(cropped, Eigen::Vector3f(1.0f, 1.0f, 1.0f), false);

        if (saveInterSteps) 
        {
            NiftiVolume::saveNifti(debugPrefix + "_resampled.nii.gz", resampledVolume);
        }

        ProgressManager::instance().report(41, 9, 85, new QString("Z-score normalization"));

        printAction("z-score normalization");
        zScoreNormalize(resampledVolume);

        if (saveInterSteps)
        {
            NiftiVolume::saveNifti(debugPrefix + "_normalized.nii.gz", resampledVolume);
        }

        ProgressManager::instance().report(41, 9, 90, new QString("Padding to minimum size 128 and multiple of 32"));

        printAction("padding to target size (128)");
        auto [padded, paddingInfo] = padVolume(resampledVolume, 128, 32);

        qInfo().noquote()
            << QString("[PADDING] X: [low:%1, high:%2], Y: [low:%3, high:%4], Z: [low:%5, high:%6]").arg(paddingInfo[0][0]).arg(paddingInfo[0][1]).arg(paddingInfo[1][0]).arg(paddingInfo[1][1]).arg(paddingInfo[2][0]).arg(paddingInfo[2][1]);

        qInfo().noquote() << QString("[FINAL SHAPE] %1x%2x%3").arg(padded.data.dimension(0)).arg(padded.data.dimension(1)).arg(padded.data.dimension(2));

        NiftiVolume::saveNifti(debugPrefix + "_PREPROC.nii.gz", padded);

        ProgressManager::instance().report(41, 9, 100);

        result.data = padded.data;
        result.spacing = padded.spacing;
        result.padding = {paddingInfo[0], paddingInfo[1], paddingInfo[2]};

        return result;
    }

    /**
     * @brief Execute the complete preprocessing pipeline for an ordered list of modalities.
     * The first modality in the list is the "reference" (defines output geometry, padding,
     * bbox used later by postprocessing). Each subsequent modality is registered to the MNI
     * atlas of its own reference family (T1 or T2). Modalities sharing a reference family with
     * an already-processed "anchor" modality reuse that anchor's dense (SVF) transform chain
     * instead of recomputing it, via BrainExtractor::runWithExistingTransform.
     * @param inputPaths Map of modality name -> file path.
     * @param modalities Ordered list of modalities (name + reference family). First = reference.
     * @param tempDir Path to the temporary/working directory for intermediate files.
     * @param betOnly If true, stop the processing after brain extraction of the reference modality.
     * @param mni If true, register to MNI space. default is false.
     * @return PreprocessedVolume Object containing the final volumes and their metadata.
     */
    PreprocessedVolume Preprocessor::preprocess(const QMap<QString, QString> &inputPaths, const QList<Modality> &modalities, const QString &tempDir, bool betOnly, bool mni, bool brainExtraction, bool saveInterSteps) 
    {
        PreprocessedVolume result;

        if (modalities.isEmpty())
        {
            throw std::runtime_error("No modalities provided for preprocessing.");
        }

        const Modality &referenceModality = modalities.first();

        if (!inputPaths.contains(referenceModality.name) || inputPaths.value(referenceModality.name).isEmpty())
        {
            throw std::runtime_error("Missing reference modality: " + referenceModality.name.toStdString());
        }

        QString t1Path = inputPaths.value(referenceModality.name);
        BrainExtractor *refExtractor = brainExtractorFor(referenceModality.reference);

        QElapsedTimer betTimer;
        betTimer.start();
        QString betT1 = t1Path;

        bool refHasTransformChain = false;
        QString refAffTrsf, refNlTrsf;

        bool refNeedsExtraction = brainExtraction && !t1Path.contains("BET") && !t1Path.contains("MNI");

        if (refNeedsExtraction) 
        {
            connect(refExtractor, &BrainExtractor::progress, [](float value, const QString &message) 
            {
                ProgressManager::instance().report(0, 41, 10 + (int)(value * 30), new QString(message));
            });

            QString prefix = tempDir + "/" + QFileInfo(t1Path).baseName();
            betT1 = refExtractor->run(t1Path, prefix);

            refAffTrsf = prefix + "_aff_tr.txt";
            refNlTrsf = prefix + "_nl_tr.nrrd";
            refHasTransformChain = true;
        }
        else 
        {
            qDebug() << "[Preprocessing] Brain extraction skipped for reference modality (already extracted or disabled by user)";
        }

        qDebug() << "Brain extraction took" << betTimer.elapsed() / 1000 << "s";

        if (betOnly) 
        {
            if (!mni) 
            {
                ProgressManager::instance().report(41, 9, 10, new QString("Registering to MNI space"));

                printAction("bias correction");
                QString prefix = tempDir + "/" + QFileInfo(betT1).baseName();
                betT1 = biasCorrect(betT1, prefix);

                printAction("registration to MNI atlas");
                auto [regPath, trsf] = registerToReference(betT1, atlasFor(referenceModality.reference), prefix, "MNI");
                betT1 = regPath;
                result.trsfPath = trsf;
            }

            result.originalPath = betT1;
        }
        else
        {
            result = preprocessModality(betT1, atlasFor(referenceModality.reference), tempDir, t1Path.contains("MNI"), nullptr, saveInterSteps);


            struct AnchorInfo
            {
                QString imgPath;
                QString affTrsf;
                QString nlTrsf;
            };
            QMap<ReferenceFamily, AnchorInfo> anchors;

            if (refHasTransformChain)
            {
                anchors[referenceModality.reference] = AnchorInfo{t1Path, refAffTrsf, refNlTrsf};
            }

            for (int i = 1; i < modalities.size(); ++i) 
            {
                const Modality &mod = modalities.at(i);
                bool hasModality = inputPaths.contains(mod.name) && !inputPaths.value(mod.name).isEmpty();

                if (hasModality)
                {
                    QString modalityPath = inputPaths.value(mod.name);
                    QString modalityTag = mod.name.toLower();
                    QString modPrefix = tempDir + "/" + modalityTag + "_bet";

                    QString modBet;
                    BrainExtractor *extractor = brainExtractorFor(mod.reference);

                    bool modNeedsExtraction = brainExtraction && !modalityPath.contains("BET") && !modalityPath.contains("MNI");

                    if (anchors.contains(mod.reference)) 
                    {
                        const AnchorInfo &anchor = anchors[mod.reference];
                        modBet = extractor->runWithExistingTransform(
                            modalityPath, modPrefix, anchor.imgPath, anchor.affTrsf, anchor.nlTrsf);
                    }
                    else if (modNeedsExtraction) 
                    {
                        modBet = extractor->run(modalityPath, modPrefix);

                        anchors[mod.reference] = AnchorInfo{
                            modalityPath,
                            modPrefix + "_aff_tr.txt",
                            modPrefix + "_nl_tr.nrrd"
                        };
                    }
                    else 
                    {
                        modBet = modalityPath;
                        qDebug() << "[Preprocessing] Brain extraction skipped for modality" << mod.name << "(already extracted or disabled by user)";
                    }

                    std::array<std::array<int, 2>, 3> modBbox = result.bbox;
                    PreprocessedVolume modResult = preprocessModality(modBet, atlasFor(mod.reference), tempDir, false, &modBbox, saveInterSteps);

                    auto t1Dims = result.data.dimensions();
                    int newC = (int)t1Dims[3] + (int)modResult.data.dimension(3);

                    Eigen::Tensor<float, 4, Eigen::ColMajor> combined((int)t1Dims[0], (int)t1Dims[1], (int)t1Dims[2], newC);

                    Eigen::array<Eigen::Index, 4> t1Ext = {(Eigen::Index)t1Dims[0], (Eigen::Index)t1Dims[1], (Eigen::Index)t1Dims[2], (Eigen::Index)t1Dims[3]};
                    combined.slice(Eigen::array<Eigen::Index, 4>{0, 0, 0, 0}, t1Ext) = result.data;

                    Eigen::array<Eigen::Index, 4> modOffset = {0, 0, 0, (Eigen::Index)t1Dims[3]};
                    Eigen::array<Eigen::Index, 4> modExt = {(Eigen::Index)t1Dims[0], (Eigen::Index)t1Dims[1], (Eigen::Index)t1Dims[2], (Eigen::Index)modResult.data.dimension(3)};
                    combined.slice(modOffset, modExt) = modResult.data;

                    result.data = combined;
                }
            }
        }

        return result;
    }

    QString Preprocessor::moveToOutput(const QString &imgPath)
    {
        QString dst;

        if (imgPath.isEmpty() || !QFile::exists(imgPath)) 
        {
            dst = imgPath;
        } 
        else 
        {
            ConfigManager &config = ConfigManager::instance();

            QString inputPath = config.get("input_path", "").toString();
            bool isFile = config.get("is_file", true).toBool();

            QString fileName = QFileInfo(imgPath).fileName();
            QString subjectName = fileName.split("_").first();

            QString outputDir;

            if (isFile)
            {
                if (inputPath.contains(QString("rawdata")))
                {
                    QString rawDir = inputPath.section(QString("rawdata"), 0, 0);
                    outputDir = rawDir + QString("derivatives") + "/" + subjectName + "/anat";
                } 
                else
                {
                    outputDir = QFileInfo(inputPath).absolutePath();
                }
            }
            else
            {
                outputDir = inputPath + "/" + QString("derivatives") + "/" + subjectName + "/anat";
            }

            QDir().mkpath(outputDir);

            dst = outputDir + "/" + fileName;

            if (!(QFileInfo(imgPath).absoluteFilePath() == QFileInfo(dst).absoluteFilePath())) 
            {
                QFile::remove(dst);
                if (!QFile::copy(imgPath, dst)) 
                {
                    qCritical() << "Failed to copy file from" << imgPath << "to" << dst;
                    dst = imgPath;
                }
            }
        }

        return dst;
    }
} // namespace preprocessing
