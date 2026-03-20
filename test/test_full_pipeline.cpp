#include <QtTest>
#include <QDir>
#include <preprocessing/preprocessor.h>
#include <inference/inferenceengine.h>
#include <postprocessing/postprocessor.h>
#include <utils/niftiVolume.h>
#include <utils/path.h>
#include "test_utils.h"

using namespace preprocessing;
using namespace postprocessing;

class TestFullPipeline : public QObject {
    Q_OBJECT

private slots:
    void testEndToEnd() {
        // 1. CONFIGURATION DES CHEMINS
        QString raw_t1 = QDir(base_dir).filePath("test/test_data/sub-r001s002-T1w.nii.gz");
        QString output_dir = QDir(base_dir).filePath("out/build/x64-Debug/full_pipeline_output");
        QString model_path = model_dir + "/model_mono_fp32.onnx";
        QDir().mkpath(output_dir);

        AnimaWrapper *wrapper = new AnimaWrapper(this);
        
        // --- STEP 1 : PREPROCESSING ---
        qDebug() << ">>> STEP 1: PREPROCESSING";
        Resampling resampler;
        QString atlas_path = atlas_dir + "/Reference_T1.nrrd"; 
        
        BrainExtraction brainExtractor(wrapper, atlas_path);
        Preprocessor preproc(&resampler, &brainExtractor, wrapper);
        
        PreprocessedVolume preproc_result;
        try {
            preproc_result = preproc.preprocess(raw_t1, "", output_dir, false);
        } catch (const std::exception &e) {
            QFAIL(qPrintable(QString("Preprocessing failed: %1").arg(e.what())));
        }

        QVERIFY(preproc_result.data.size() > 0);
        QString preproc_file = output_dir + "/preprocessed_input.nii.gz";
        NiftiVolume vol_preproc;
        vol_preproc.data = preproc_result.data;
        vol_preproc.spacing = preproc_result.spacing;
        NiftiVolume::saveNifti(preproc_file, vol_preproc);

        // --- STEP 2 : INFERENCE ---
        qDebug() << ">>> STEP 2: INFERENCE";
        InferenceEngine engine;
        QString inference_out_path = output_dir + "/inference_raw_output.nii.gz";
        
        NiftiVolume inference_result;
        try {
            inference_result = engine.run(model_path, preproc_file, inference_out_path, "input", "output");
        } catch (const std::exception &e) {
            QFAIL(qPrintable(QString("Inference failed: %1").arg(e.what())));
        }
        QVERIFY(inference_result.data.size() > 0);

        // --- STEP 3 : POSTPROCESSING ---
        qDebug() << ">>> STEP 3: POSTPROCESSING";
        Postprocessor postproc(wrapper);
        
        auto bbox = preproc_result.bbox; 
        
        try {
            postproc.postprocess(
                inference_result.data,
                preproc_result,
                bbox,
                0.5f,   // threshold
                true,   // save_pmap
                output_dir,
                preproc_result.trsf_path
            );
        } catch (const std::exception &e) {
            QFAIL(qPrintable(QString("Postprocessing failed: %1").arg(e.what())));
        }

        // --- STEP 4 : FAINAL VALIDATION ---
        QString final_path = output_dir + "/segmentation_patient_space.nii.gz";
        QVERIFY2(QFile::exists(final_path), "Le fichier final en espace patient n'a pas été généré.");

        NiftiVolume final_vol = NiftiVolume::loadNifti(final_path);
        
        NiftiVolume original_vol = NiftiVolume::loadNifti(raw_t1);
        QCOMPARE(final_vol.data.dimension(1), original_vol.data.dimension(1));
        QCOMPARE(final_vol.data.dimension(2), original_vol.data.dimension(2));
        QCOMPARE(final_vol.data.dimension(3), original_vol.data.dimension(3));

        qDebug() << ">>> FULL SUCCESSFUL!";
    }
};

QTEST_MAIN(TestFullPipeline);
#include "test_full_pipeline.moc"