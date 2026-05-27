#include "pipelineWorker.h"
#include <utils/dicomConverter.h>

#include <QElapsedTimer>

#include <inference/inference.h>
#include <managers/progressManager.h>

int PipelineWorker::process() {
    try {
        ProgressManager::instance().reset();

        ProgressManager::instance().setFileName(QFileInfo(m_p.t1Path).fileName());

        QElapsedTimer total_timer;
        QElapsedTimer step_timer;
        total_timer.start();

        QString workingInputPath = m_p.t1Path;
        bool isTemporaryInput = false;

        if (DicomConverter::requiresConversion(m_p.t1Path)) {
            emit statusChanged("Conversion DICOM en cours...");
            qDebug() << "[DICOM] Detection d'un format non-NIfTI. Lancement de dcm2niix.";

            // On convertit dans le dossier de sortie
            QString convertedFile =
                DicomConverter::convert(m_p.t1Path, m_p.outputDir);

            if (convertedFile.isEmpty() || !QFile::exists(convertedFile)) {
                throw std::runtime_error("La conversion DICOM a échoué. Vérifiez l'intégrité des "
                                         "fichiers ou la présence de dcm2niix.");
            }

            workingInputPath = convertedFile;
            isTemporaryInput = true;
            qDebug() << "[DICOM] Conversion réussie :" << workingInputPath;
        }


        emit statusChanged("Initialisation des composants...");

        preprocessing::Resampling resampler;
        BrainExtraction brainExtractor(m_wrapper, Paths::atlasDir().filePath("Reference_T1.nii.gz"));
        preprocessing::Preprocessor preproc(&resampler, &brainExtractor, m_wrapper, true);
        PreprocessedVolume preprocResult;
        NiftiVolume inferenceResult;

        QString baseName = QFileInfo(m_p.t1Path).baseName();
        QString preprocPath = m_p.outputDir + "/" + baseName + "_BET_PREPROC.nii.gz";
        QString rawInferencePath = m_p.outputDir + "/" + baseName + "_INFERENCE_raw.nii.gz";

        QString metaPath = m_p.outputDir + "/" + baseName + "_PREPROC_metadata.json";
        
        QString prefixMNI = m_p.mni ? "MNI_" : "";
        QString thresholdStr = m_p.threshold == 0.5 ? "" : "_" + QString::number(m_p.threshold * 100, 'f', 3).replace(".", "");

        // Remove leading underscores from suffix to avoid double underscores in filename
        if (m_p.suffix.startsWith("_")) {
            m_p.suffix = m_p.suffix.mid(1);
        }

        QString finalFileName = prefixMNI + QFileInfo(m_p.t1Path).baseName() + thresholdStr + "_seg" + ".nii.gz";
        QString finalPath = m_p.outputDir + "/" + finalFileName;

        bool bypassed_preproc = m_p.skipPreProcessing;
        bool bypassed_inference = m_p.skipInference;
        bool bypassed_postproc = m_p.skipPostProcessing;

        // 1. PREPROCESSING

        step_timer.start();

        if (bypassed_preproc && QFile::exists(preprocPath) && QFile::exists(metaPath)) {
            emit statusChanged("Cache detected, preprocessed volume loading...");
            qDebug() << "[BYPASS] Loading existing preprocessed file:" << preprocPath;
            NiftiVolume existingVol = NiftiVolume::loadNifti(preprocPath);

            preprocResult.data = existingVol.data;
            preprocResult.spacing = existingVol.spacing;

            preprocResult.loadMetadata(metaPath);
            qDebug() << "[BYPASS] Preprocessing skipped. Using cached files.";

            ProgressManager::instance().report(41, 0, 100, new QString("Bypassing preprocessing"));

            if (ProgressManager::instance().isInterrupted()) {
                throw std::runtime_error("Cancelled");
            }
        } else {
            qDebug() << "[BYPASS] No valid cache found. Running preprocessing.";
            bypassed_preproc = false;
        }

        if (!bypassed_preproc) {
            emit statusChanged("Step 1/3 : Preprocessing...");
            preprocessing::Preprocessor preproc(&resampler, &brainExtractor, m_wrapper,
                                                m_p.savePreProcessing);

            preprocResult = preproc.preprocess(m_p.t1Path, "", m_p.outputDir, m_p.betOnly, m_p.mni);
            preprocResult.saveMetadata(metaPath);

            if (m_p.savePreProcessing) {
                QString preprocSavePath = m_p.outputDir + "/debug_preprocessed.nii.gz";
                NiftiVolume volPre;
                volPre.data = preprocResult.data;
                volPre.spacing = preprocResult.spacing;
                NiftiVolume::saveNifti(preprocSavePath, volPre);
            }

            if (m_p.betOnly) {
                emit finished(true, "Brain extraction performed with success !", preprocPath);
                return 0;
            }

            qDebug() << "------------------------------------------";
            qDebug() << "[TIMER] PREPROCESSING :" << step_timer.elapsed() / 1000 << "s";
            qDebug() << "------------------------------------------";

            if (ProgressManager::instance().isInterrupted()) {
                throw std::runtime_error("Cancelled");
            }
        }

        // 2. INFERENCE
        if (bypassed_inference && QFile::exists(rawInferencePath)) {
            emit statusChanged("Cache detected, inference result loading...");
            qDebug() << "[BYPASS] Loading existing inference result:" << rawInferencePath;
            inferenceResult = NiftiVolume::loadNifti(rawInferencePath);
            ProgressManager::instance().report(41, 1, 100, new QString("Bypassing inference"));
            if (ProgressManager::instance().isInterrupted()) {
                throw std::runtime_error("Cancelled");
            }
        } else {
            qDebug() << "[BYPASS] No valid cache found. Running inference.";
            bypassed_inference = false;
        }

        if (!bypassed_inference) {
            step_timer.restart();
            Inference engine;
            emit statusChanged("Step 2/3 : Inference...");

            NiftiVolume::saveNifti(preprocPath, {preprocResult.data, preprocResult.spacing});

            NiftiVolume inferenceInput = NiftiVolume::loadNifti(preprocPath);

            inferenceResult =
                engine.run(m_p.modelPath, inferenceInput, rawInferencePath, "input", "output");

            qDebug() << "------------------------------------------";
            qDebug() << "[TIMER] INFERENCE :" << step_timer.elapsed() / 1000 << "s";
            qDebug() << "------------------------------------------";
        }

        if (ProgressManager::instance().isInterrupted()) {
            throw std::runtime_error("Cancelled");
        }

        // 3. POSTPROCESSING

        if (bypassed_postproc && QFile::exists(finalPath)) {
            emit statusChanged("Cache detected, postprocessed volume loading...");
            qDebug() << "[BYPASS] Loading existing postprocessed file:" << finalPath;
            NiftiVolume final_volume = NiftiVolume::loadNifti(finalPath);
            emit finished(true, "analysis performed with success !", finalPath);
            return 0;
        } else {
            qDebug() << "[BYPASS] No valid cache found. Running postprocessing.";
            bypassed_postproc = false;
        }

        if (!bypassed_postproc) {
            step_timer.restart();
            emit statusChanged("Step 3/3 : Postprocessing...");

            postprocessing::Postprocessor postproc(m_wrapper);

            NiftiVolume final_volume = postproc.postprocess(
                inferenceResult.data, preprocResult, preprocResult.bbox, m_p.threshold,
                m_p.savePMap, m_p.outputDir, preprocResult.trsf_path, m_p.mni);

            // Duplicate original header to avoid header corruption.
            if (m_p.mni) {
                NiftiVolume::saveNiftiWithReference(
                    finalPath, final_volume, Paths::atlasDir().filePath("Reference_T1.nii.gz"));
            } else {
                qDebug() << finalPath;
                qDebug() << finalFileName;
                NiftiVolume::saveNiftiWithReference(finalPath, final_volume, m_p.t1Path);
            }

            emit finished(true, "analysis performed with success !", finalPath);

            qDebug() << "------------------------------------------";
            qDebug() << "[TIMER] POSTPROCESSING :" << step_timer.elapsed() / 1000 << "s";
            qDebug() << "------------------------------------------";
        }

        qDebug() << "------------------------------------------";
        qDebug() << "[TIMER] TOTAL PROCESS :" << total_timer.elapsed() / 1000 << "s";
        qDebug() << "------------------------------------------";

        if (ProgressManager::instance().isInterrupted()) {
            throw std::runtime_error("Cancelled");
        }

        ProgressManager::instance().reset();

        return 0;
    } catch (const std::runtime_error &e) {
        emit finished(false, QString("Error : %1").arg(e.what()), "");
        return -1;
    } catch (const std::exception &e) {
        emit finished(false, QString("Fatal Error : %1").arg(e.what()), "");
        return -1;
    } catch (...) {
        emit finished(false, "Unknown error occured during pipeline.", "");
        return -1;
    }
}

PipelineWorker::PipelineWorker(PipelineParams &p) {
    m_p = p;
    m_wrapper = new AnimaWrapper(this);

    connect(&ProgressManager::instance(), &ProgressManager::interruptionRequested,
            m_wrapper, &AnimaWrapper::abort, Qt::DirectConnection);
}
