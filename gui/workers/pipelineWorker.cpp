#include "pipelineWorker.h"
#include <inference/inference.h>
#include <QElapsedTimer>

void PipelineWorker::process() {
    try {

        QElapsedTimer total_timer;
        QElapsedTimer step_timer;
        total_timer.start();

        emit statusChanged("Initialisation des composants...");

        AnimaWrapper wrapper(this);
        preprocessing::Resampling resampler;
        BrainExtraction brainExtractor(&wrapper, Paths::atlasDir() + "Reference_T1.nrrd");
        preprocessing::Preprocessor preproc(&resampler, &brainExtractor, &wrapper, true);
        PreprocessedVolume preprocResult;

        QString baseName = QFileInfo(m_p.t1Path).baseName();
        QString preprocPath = m_p.outputDir + "/" + baseName + "_BET_PREPROC.nii.gz";
        QString metaPath = m_p.outputDir + "/" + baseName + "_PREPROC_metadata.json";

        bool bypassed = false;

        // 1. PREPROCESSING

        step_timer.start();

        if (QFile::exists(preprocPath) && QFile::exists(metaPath)) {
            emit statusChanged("Cache detected, preprocessed volume loading...");
            qDebug() << "[BYPASS] Loading existing preprocessed file:" << preprocPath;
            // On charge le volume existant
            NiftiVolume existingVol = NiftiVolume::loadNifti(preprocPath);

            // On remplit l'objet preprocResult manuellement
            preprocResult.data = existingVol.data;
            preprocResult.spacing = existingVol.spacing;

            if (preprocResult.loadMetadata(metaPath)) {
                bypassed = true;
                qDebug() << "[BYPASS] Preprocessing skipped. Using cached files.";
            }
        }

        if (!bypassed) {
            emit statusChanged("Step 1/3 : Preprocessing...");
            preprocessing::Preprocessor preproc(&resampler, &brainExtractor, &wrapper,
                                                m_p.savePreproc);
            preprocResult = preproc.preprocess(m_p.t1Path, "", m_p.outputDir, m_p.skipBrainExtract);
            preprocResult.saveMetadata(metaPath);

            if (m_p.savePreproc) {
                QString preprocSavePath = m_p.outputDir + "/debug_preprocessed.nii.gz";
                NiftiVolume volPre;
                volPre.data = preprocResult.data;
                volPre.spacing = preprocResult.spacing;
                NiftiVolume::saveNifti(preprocSavePath, volPre);
            }

            qDebug() << "------------------------------------------";
            qDebug() << "[TIMER] PREPROCESSING :" << step_timer.elapsed() / 1000 << "s";
            qDebug() << "------------------------------------------";
        }

        // 2. INFERENCE
        step_timer.restart();
        Inference engine;
        emit statusChanged("Step 2/3 : Inference...");

        QString tmpInput = m_p.outputDir + "/tmp_inference_input.nii.gz";
        NiftiVolume::saveNifti(tmpInput, {preprocResult.data, preprocResult.spacing});

        NiftiVolume inferenceVol = NiftiVolume::loadNifti(tmpInput);

        QString rawInferencePath = m_p.outputDir + "/inference_raw.nii.gz";
        auto inferenceResult =
            engine.run(m_p.modelPath, inferenceVol, rawInferencePath, "input", "output");

        qDebug() << "------------------------------------------";
        qDebug() << "[TIMER] INFERENCE :" << step_timer.elapsed() / 1000 << "s";
        qDebug() << "------------------------------------------";

        // 3. POSTPROCESSING
        step_timer.restart();
        emit statusChanged("Step 3/3 : Postprocessing...");

        postprocessing::Postprocessor postproc(&wrapper);

        QString fileName = QFileInfo(m_p.t1Path).baseName() + m_p.suffix + ".nii.gz";
        QString finalPath = m_p.outputDir + "/" + fileName;

        postproc.postprocess(
            inferenceResult.data, preprocResult,
            preprocResult.bbox, 
            m_p.threshold, m_p.savePMap, m_p.outputDir, preprocResult.trsf_path);

        QFile::remove(tmpInput);
        if (!m_p.savePreproc)
            QFile::remove(rawInferencePath);

        emit finished(true, "analysis performed with success !", finalPath);

        qDebug() << "------------------------------------------";
        qDebug() << "[TIMER] POSTPROCESSING :" << step_timer.elapsed() / 1000 << "s";
        qDebug() << "------------------------------------------";

        qDebug() << "------------------------------------------";
        qDebug() << "[TIMER] TOTAL PROCESS :" << total_timer.elapsed() / 1000 << "s";
        qDebug() << "------------------------------------------";

    } catch (const std::exception &e) {
        emit finished(false, QString("Fatal Error : %1").arg(e.what()), "");
    } catch (...) {
        emit finished(false, "Unknown error occured during pipeline.", "");
    }
}