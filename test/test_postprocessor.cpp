#include <QDir>
#include <QtTest>
#include <postprocessing/postprocessor.h>
#include <utils/niftiVolume.h>
#include <utils/path.h>
#include "test_utils.h"

using namespace postprocessing;

class TestPostprocessor : public QObject {
    Q_OBJECT

private slots:
    void testIntegrationPostprocessing() {
        // 1. Chemins
        QString inputPath = QDir(base_dir).filePath("test/test_data/inference_output.nii.gz");
        QString outputDir = QDir(base_dir).filePath("out/build/x64-Debug/output_data");
        QDir().mkpath(outputDir);

        qDebug() << "Demarrage du test de postprocessing...";
        
        // 2. Charger les données d'inférence (simulées ou réelles)
        // On suppose que inference_output.nii.gz est un volume 4D [X, Y, Z, C]
        NiftiVolume inference_vol = NiftiVolume::loadNifti(inputPath);
        QVERIFY(!inference_vol.data.size() == 0);

        // 3. Simuler les métadonnées de pré-traitement
        // Ces données sont normalement générées par le Preprocessor
        PreprocessedVolume preproc;
        preproc.original_shape = Eigen::Vector3i(256, 256, 176); // Taille originale du patient
        
        // Padding appliqué lors du pré-traitement (exemple: 4 pixels de chaque côté)
        preproc.padding = {
            std::array<int, 2>{4, 124}, // X: on garde de l'index 4 à 124 (sur 128)
            std::array<int, 2>{4, 124}, // Y
            std::array<int, 2>{4, 124}  // Z
        };

        // Bounding box (où le cerveau se trouvait dans l'image originale)
        std::array<std::array<int, 2>, 3> bbox = {
            std::array<int, 2>{60, 180}, 
            std::array<int, 2>{60, 180}, 
            std::array<int, 2>{20, 140}
        };

        // 4. Initialiser le Postprocessor
        Postprocessor postprocessor;
        float threshold = 0.5f;
        bool save_pmap = true;

        // 5. Exécuter le pipeline
        try {
            postprocessor.postprocess(
                inference_vol.data, 
                preproc, 
                bbox, 
                threshold, 
                save_pmap, 
                outputDir, 
                "" // trsf_path si nécessaire
            );
        } catch (const std::exception& e) {
            QFAIL(qPrintable(QString("Le postprocessing a crashé : %1").arg(e.what())));
        }

        // 6. Vérifications
        //QString expectedFile = outputDir + "/azerty_pmap.nii.gz";
        //QVERIFY2(QFile::exists(expectedFile), "Le fichier de sortie n'a pas été généré.");

        // Charger le résultat pour vérifier la cohérence spatiale
        //NiftiVolume result = NiftiVolume::loadNifti(expectedFile);
        
        // Vérifier que la taille correspond bien à l'original_shape (Step 3: Uncrop)
        //QCOMPARE(result.data.dimension(0), (Eigen::Index)preproc.original_shape[0]);
        //QCOMPARE(result.data.dimension(1), (Eigen::Index)preproc.original_shape[1]);
        //QCOMPARE(result.data.dimension(2), (Eigen::Index)preproc.original_shape[2]);

        //qDebug() << "Test réussi : Volume reconstruit en" 
        //         << result.data.dimension(0) << "x" 
        //         << result.data.dimension(1) << "x" 
        //         << result.data.dimension(2);
    }
};

QTEST_MAIN(TestPostprocessor);
#include "test_postprocessor.moc"