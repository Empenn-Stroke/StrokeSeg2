#include <QDir>
#include <QtTest>

#include <inference/inferenceengine.h>
#include <utils/path.h>

#include "test_utils.h"

class TestInference : public QObject {
    Q_OBJECT

  private slots:

    void testInference() {
        InferenceEngine engine;

        QString modelPath = model_dir + "/model_mono_fp32.onnx";
        QString inputPath = QDir(base_dir).filePath("test/test_data/final_processed_result.nii.gz");
        QString outputPath = QDir(base_dir).filePath("test/test_data/inference_output.nii.gz");

        qDebug() << "Demarrage du test d'inference...";
        qDebug() << "Input :" << inputPath;
        qDebug() << "Model :" << modelPath;

        NiftiVolume result = engine.run(modelPath, inputPath, outputPath, "input", "output");

        if (result.data.size() > 0) {
            qDebug() << "Test reussi ! Le fichier a ete sauvegarde ici :" << outputPath;
        } else {
            qCritical() << "Le test a echoue. Verifie les logs ONNX dans la console.";
        }
    }
};

QTEST_MAIN(TestInference);
#include "test_inference.moc"