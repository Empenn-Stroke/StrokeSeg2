// SPDX-License-Identifier: AGPL-3.0-or-later

#include "pipelineWorker.h"
#include <utils/dicomConverter.h>

#include <QDir>
#include <QElapsedTimer>

#include <inference/inference.h>
#include <inference/model.h>
#include <managers/progressManager.h>

/**
 * @brief RAII guard that removes the given temporary directory (and all its
 * content) when it goes out of scope, regardless of whether process() returns
 * normally or an exception unwinds the stack. Ensures intermediate files
 * (registration, bias correction, brain extraction byproducts, etc.) never
 * accumulate across runs, while files explicitly copied out of tempDir before
 * destruction (transform file, original BET volume) remain untouched.
 */
struct TempDirGuard 
{
    QString path;

    ~TempDirGuard() 
    {
        if (!path.isEmpty() && QDir(path).exists()) 
        {
            bool removed = QDir(path).removeRecursively();
            if (!removed) 
            {
                qWarning() << "[TempCleanup] Failed to remove temp directory:" << path;
            } 
            else 
            {
                qDebug() << "[TempCleanup] Removed temp directory:" << path;
            }
        }
    }
};

int PipelineWorker::process() 
{
    int resultCode = 0;

    try 
    {
        ProgressManager::instance().reset();

        if (m_p.inputOrder.isEmpty() || !m_p.inputPaths.contains(m_p.inputOrder.first())) 
        {
            throw std::runtime_error("No reference input defined for this model.");
        }

        for (auto it = m_p.inputPaths.constBegin(); it != m_p.inputPaths.constEnd(); ++it) {
            const QString &key = it.key();
            const QString &path = it.value();

            if (!path.isEmpty() && !QFile::exists(path)) {
                throw std::runtime_error(QString("Input file for %1 does not exist: %2").arg(key, path).toStdString());
            }
        }

        QString referenceKey = m_p.inputOrder.first();
        QString referencePath = m_p.inputPaths.value(referenceKey);

        ProgressManager::instance().setFileName(QFileInfo(referencePath).fileName());

        QElapsedTimer total_timer;
        QElapsedTimer step_timer;
        total_timer.start();

        QMap<QString, QString> workingPaths = m_p.inputPaths;

        for (auto it = m_p.inputPaths.constBegin(); it != m_p.inputPaths.constEnd(); ++it) 
        {
            const QString &key = it.key();
            const QString &path = it.value();
            bool needsConversion = !path.isEmpty() && DicomConverter::requiresConversion(path);

            if (needsConversion)
            {
                emit statusChanged(QString("DICOM Conversion %1 for...").arg(key));
                qDebug() << "[DICOM] non-NIfTI detected for" << key << ". Launching dcm2niix.";

                QString convertedFile = DicomConverter::convert(path, m_p.outputDir);

                if (convertedFile.isEmpty() || !QFile::exists(convertedFile)) 
                {
                    throw std::runtime_error(QString("The DICOM conversion of %1 failed. Please check the file integrity or the presence of dcm2niix.").arg(key).toStdString());
                }

                workingPaths[key] = convertedFile;
                qDebug() << "[DICOM] Conversion successful for" << key << ":" << convertedFile;
            }
        }

        m_p.inputPaths = workingPaths;
        referencePath = m_p.inputPaths.value(referenceKey);

        emit statusChanged("Component initialization...");

        preprocessing::Resampler resampler;
        BrainExtractor brainExtractorT1(m_wrapper, Paths::atlasDir().filePath("Reference_T1.nii.gz"));
        BrainExtractor brainExtractorT2(m_wrapper, Paths::atlasDir().filePath("Reference_T2.nii.gz"));
        preprocessing::Preprocessor preproc(&resampler, &brainExtractorT1, &brainExtractorT2, m_wrapper);
        PreprocessedVolume preprocResult;
        NiftiVolume inferenceResult;

        QString baseName = QFileInfo(referencePath).baseName();
        QString preprocPath = m_p.outputDir + "/" + baseName + "_BET_PREPROC.nii.gz";
        QString rawInferencePath = m_p.outputDir + "/" + baseName + "_INFERENCE_raw.nii.gz";
        QString metaPath = m_p.outputDir + "/" + baseName + "_PREPROC_metadata.json";

        QString prefixMNI = m_p.mni ? "MNI_" : "";
        QString thresholdStr = m_p.threshold == 0.5 ? "" : "_" + QString::number(m_p.threshold * 100, 'f', 3).replace(".", "");

        if (m_p.suffix.startsWith("_")) 
        {
            m_p.suffix = m_p.suffix.mid(1);
        }

        QString finalFileName = prefixMNI + baseName + thresholdStr + "_seg" + ".nii.gz";
        QString finalPath = m_p.outputDir + "/" + finalFileName;

        bool bypassed_preproc = m_p.skipPreProcessing;
        bool bypassed_inference = m_p.skipInference;
        bool bypassed_postproc = m_p.skipPostProcessing;
        bool betOnlyDone = false;

        // 1. PREPROCESSING
        step_timer.start();

        if (bypassed_preproc && QFile::exists(preprocPath) && QFile::exists(metaPath)) 
        {
            emit statusChanged("Cache detected, preprocessed volume loading...");
            qDebug() << "[BYPASS] Loading existing preprocessed file:" << preprocPath;
            NiftiVolume existingVol = NiftiVolume::loadNifti(preprocPath);

            preprocResult.data = existingVol.data;
            preprocResult.spacing = existingVol.spacing;
            preprocResult.loadMetadata(metaPath);

            qDebug() << "[BYPASS] Preprocessing skipped. Using cached files.";

            int loadedX = (int)existingVol.data.dimension(0);
            int loadedY = (int)existingVol.data.dimension(1);
            int loadedZ = (int)existingVol.data.dimension(2);

            int maxPadX = preprocResult.padding[0][1];
            int maxPadY = preprocResult.padding[1][1];
            int maxPadZ = preprocResult.padding[2][1];

            bool cacheConsistent = (maxPadX <= loadedX) && (maxPadY <= loadedY) && (maxPadZ <= loadedZ);

            if (!cacheConsistent) 
            {
                throw std::runtime_error(QString("Cache incohérent détecté (dimensions %1x%2x%3 vs padding attendu jusqu'à %4x%5x%6). "
                                                 "Relancez sans 'Skip pre-processing' pour régénérer le cache.")
                                             .arg(loadedX)
                                             .arg(loadedY)
                                             .arg(loadedZ)
                                             .arg(maxPadX)
                                             .arg(maxPadY)
                                             .arg(maxPadZ)
                                             .toStdString());
            }

            ProgressManager::instance().report(41, 0, 100, new QString("Bypassing preprocessing"));

            if (ProgressManager::instance().isInterrupted()) 
            {
                throw std::runtime_error("Cancelled");
            }
        } 
        else 
        {
            qDebug() << "[BYPASS] No valid cache found. Running preprocessing.";
            bypassed_preproc = false;
        }

        if (!bypassed_preproc) 
        {
            emit statusChanged("Step 1/3 : Preprocessing...");

            QString tempDir = m_p.saveInterSteps ? m_p.outputDir : (m_p.outputDir + "/_tmp");
            QDir().mkpath(tempDir);

            TempDirGuard tempGuard{m_p.saveInterSteps ? QString() : tempDir};

            preprocResult = preproc.preprocess(m_p.inputPaths, m_p.modalities, tempDir, m_p.betOnly, m_p.mni, m_p.brainExtraction, m_p.saveInterSteps);

            if (!preprocResult.trsfPath.isEmpty() && QFile::exists(preprocResult.trsfPath)) 
            {
                QString persistentTrsfPath = m_p.outputDir + "/" + QFileInfo(preprocResult.trsfPath).fileName();

                bool copied = QFile::exists(persistentTrsfPath) ? true : QFile::copy(preprocResult.trsfPath, persistentTrsfPath);

                if (copied) 
                {
                    preprocResult.trsfPath = persistentTrsfPath;
                } 
                else 
                {
                    qWarning() << "[Pipeline] Failed to persist transform file:" << preprocResult.trsfPath;
                }
            }

            if (!preprocResult.originalPath.isEmpty() && QFile::exists(preprocResult.originalPath)) 
            {
                QString persistentOriginalPath = m_p.outputDir + "/" + QFileInfo(preprocResult.originalPath).fileName();

                bool copied = QFile::exists(persistentOriginalPath) ? true : QFile::copy(preprocResult.originalPath, persistentOriginalPath);

                if (copied) 
                {
                    preprocResult.originalPath = persistentOriginalPath;
                } 
                else 
                {
                    qWarning() << "[Pipeline] Failed to persist original BET file:" << preprocResult.originalPath;
                }
            }

            preprocResult.saveMetadata(metaPath);

            if (m_p.saveInterSteps) 
            {
                QString preprocSavePath = m_p.outputDir + "/debug_preprocessed.nii.gz";
                NiftiVolume volPre;
                volPre.data = preprocResult.data;
                volPre.spacing = preprocResult.spacing;
                NiftiVolume::saveNifti(preprocSavePath, volPre);
            }

            if (m_p.betOnly) 
            {
                emit finished(true, "Brain extraction performed with success !", preprocPath);
                betOnlyDone = true;
            }

            if (!betOnlyDone)
            {
                qDebug() << "------------------------------------------";
                qDebug() << "[TIMER] PREPROCESSING :" << step_timer.elapsed() / 1000 << "s";
                qDebug() << "------------------------------------------";

                if (ProgressManager::instance().isInterrupted()) 
                {
                    throw std::runtime_error("Cancelled");
                }
            }

            // tempGuard removes _tmp here, at end of scope, whether or not an exception occurs below
        }

        if (!betOnlyDone)
        {
            // 2. INFERENCE
            if (bypassed_inference && QFile::exists(rawInferencePath)) 
            {
                emit statusChanged("Cache detected, inference result loading...");
                qDebug() << "[BYPASS] Loading existing inference result:" << rawInferencePath;
                inferenceResult = NiftiVolume::loadNifti(rawInferencePath);
                ProgressManager::instance().report(41, 1, 100, new QString("Bypassing inference"));
                if (ProgressManager::instance().isInterrupted()) 
                {
                    throw std::runtime_error("Cancelled");
                }
            } 
            else 
            {
                qDebug() << "[BYPASS] No valid cache found. Running inference.";
                bypassed_inference = false;
            }

            if (!bypassed_inference) 
            {
                step_timer.restart();
                Inference engine;
                emit statusChanged("Step 2/3 : Inference...");

                NiftiVolume::saveNifti(preprocPath, {preprocResult.data, preprocResult.spacing});

                NiftiVolume inferenceInput = NiftiVolume::loadNifti(preprocPath);

                auto model = std::make_shared<Model>();
                if (!model->load(m_p.modelPath)) 
                {
                    throw std::runtime_error("Failed to load the ONNX model. Please check the model path and file integrity.");
                }

                inferenceResult = engine.run(model, inferenceInput, rawInferencePath, "input", "output");

                qDebug() << "------------------------------------------";
                qDebug() << "[TIMER] INFERENCE :" << step_timer.elapsed() / 1000 << "s";
                qDebug() << "------------------------------------------";
            }

            if (ProgressManager::instance().isInterrupted()) 
            {
                throw std::runtime_error("Cancelled");
            }

            // 3. POSTPROCESSING
            bool postprocDoneFromCache = bypassed_postproc && QFile::exists(finalPath);

            if (postprocDoneFromCache) 
            {
                emit statusChanged("Cache detected, postprocessed volume loading...");
                qDebug() << "[BYPASS] Loading existing postprocessed file:" << finalPath;
                NiftiVolume final_volume = NiftiVolume::loadNifti(finalPath);
                emit finished(true, "analysis performed with success !", finalPath);
            } 
            else 
            {
                qDebug() << "[BYPASS] No valid cache found. Running postprocessing.";

                step_timer.restart();
                emit statusChanged("Step 3/3 : Postprocessing...");

                postprocessing::Postprocessor postproc(m_wrapper);

                NiftiVolume final_volume = postproc.postprocess(
                    inferenceResult.data, preprocResult, preprocResult.bbox, m_p.threshold,
                    m_p.savePMap, m_p.outputDir, preprocResult.trsfPath, m_p.mni, m_p.saveInterSteps);

                if (m_p.mni) 
                {
                    NiftiVolume::saveNiftiWithReference(
                        finalPath, final_volume, Paths::atlasDir().filePath("Reference_T1.nii.gz"));
                } 
                else 
                {
                    qDebug() << finalPath;
                    qDebug() << finalFileName;
                    NiftiVolume::saveNiftiWithReference(finalPath, final_volume, referencePath);
                }

                emit finished(true, "analysis performed with success !", finalPath);

                qDebug() << "------------------------------------------";
                qDebug() << "[TIMER] POSTPROCESSING :" << step_timer.elapsed() / 1000 << "s";
                qDebug() << "------------------------------------------";
            }

            qDebug() << "------------------------------------------";
            qDebug() << "[TIMER] TOTAL PROCESS :" << total_timer.elapsed() / 1000 << "s";
            qDebug() << "------------------------------------------";

            if (ProgressManager::instance().isInterrupted()) 
            {
                throw std::runtime_error("Cancelled");
            }

            ProgressManager::instance().reset();
        }
    } 
    catch (const std::runtime_error &e) 
    {
        emit finished(false, QString("Error : %1").arg(e.what()), "");
        resultCode = -1;
    } 
    catch (const std::exception &e) 
    {
        emit finished(false, QString("Fatal Error : %1").arg(e.what()), "");
        resultCode = -1;
    } 
    catch (...) 
    {
        emit finished(false, "Unknown error occured during pipeline.", "");
        resultCode = -1;
    }

    return resultCode;
}

PipelineWorker::PipelineWorker(PipelineParams &p) 
{
    m_p = p;
    m_wrapper = new AnimaWrapper(this);

    connect(&ProgressManager::instance(), &ProgressManager::interruptionRequested, m_wrapper, &AnimaWrapper::abort, Qt::DirectConnection);
}
