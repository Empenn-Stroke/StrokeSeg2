#include "pipelineWorker.h"
#include <QElapsedTimer>

void PipelineWorker::process() {
    try {
        QElapsedTimer total_timer;
        total_timer.start();

        emit statusChanged("Initialisation des composants...");

        AnimaWrapper wrapper(this);
        preprocessing::Resampling resampler;

        BrainExtraction brainExtractor(&wrapper,
                                       "C:/ProgramData/StrokeSeg/Atlas/Reference_T1.nii.gz");

        preprocessing::Preprocessor preproc(&resampler, &brainExtractor, &wrapper, false);
        InferenceEngine engine;
        postprocessing::Postprocessor postproc(&wrapper);

        // 2. PREPROCESSING
        emit statusChanged("Step 1/3 : Preprocessing...");

        auto preprocResult =
            preproc.preprocess(m_p.t1Path, "", m_p.outputDir, m_p.skipBrainExtract);

        if (m_p.savePreproc) {
            QString preprocSavePath = m_p.outputDir + "/debug_preprocessed.nii.gz";
            NiftiVolume volPre;
            volPre.data = preprocResult.data;
            volPre.spacing = preprocResult.spacing;
            NiftiVolume::saveNifti(preprocSavePath, volPre);
        }

        // 3. INFERENCE
        emit statusChanged("Step 2/3 : Inference...");

        QString tmpInput = m_p.outputDir + "/tmp_inference_input.nii.gz";
        NiftiVolume::saveNifti(tmpInput, {preprocResult.data, preprocResult.spacing});

        QString rawInferencePath = m_p.outputDir + "/inference_raw.nii.gz";
        auto inferenceResult =
            engine.run(m_p.modelPath, tmpInput, rawInferencePath, "input", "output");

        // 4. POSTPROCESSING
        emit statusChanged("Step 3/3 : Postprocessing...");

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
        qDebug() << "[TIMER] TOTAL PROCESS:" << total_timer.elapsed() / 1000 << "s";
        qDebug() << "------------------------------------------";

    } catch (const std::exception &e) {
        emit finished(false, QString("Erreur fatale : %1").arg(e.what()), "");
    } catch (...) {
        emit finished(false, "Unknown error occured during pipeline.", "");
    }
}