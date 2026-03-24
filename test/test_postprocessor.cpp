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
        QString t1_base_path = QDir(base_dir).filePath("test/test_data/sub-r001s002-T1w.nii.gz");

        QString inputPath = QDir(base_dir).filePath("test/test_data/inference_output.nii.gz");
        QString outputDir = QDir(base_dir).filePath("out/build/x64-Debug/output_data");
        QString trsf_path = QDir(outputDir).filePath("MNI_t1_BET.txt");
        QDir().mkpath(outputDir);

        qDebug() << "Demarrage du test de postprocessing...";
        
        // 2. Charger les données d'inférence (simulées ou réelles)
        // On suppose que inference_output.nii.gz est un volume 4D [X, Y, Z, C]
        NiftiVolume inference_vol = NiftiVolume::loadNifti(inputPath);
        QVERIFY(!inference_vol.data.size() == 0);

        // 3. Simuler les métadonnées de pré-traitement
        // Ces données sont normalement générées par le Preprocessor
        PreprocessedVolume preproc;

        preproc.original_t1_path = t1_base_path; // Chemin vers le T1 original

        preproc.original_shape = Eigen::Vector3i(160, 256, 256); // Taille originale du patient

        preproc.spacing = Eigen::Vector3f(1.0f, 1.0f, 1.0f); // Espacement en mm (exemple)
        
        // Padding appliqué lors du pré-traitement (exemple: 4 pixels de chaque côté)
        preproc.padding = {
            std::array<int, 2>{11, 149}, // X
            std::array<int, 2>{9, 183}, // Y
            std::array<int, 2>{10, 149}  // Z
        };

        // Bounding box (où le cerveau se trouvait dans l'image originale)
        std::array<std::array<int, 2>, 3> bbox = {
            std::array<int, 2>{10, 148}, 
            std::array<int, 2>{60, 234}, 
            std::array<int, 2>{65, 204}
        };

        AnimaWrapper *wrapper = new AnimaWrapper(this);

        // 4. Initialiser le Postprocessor
        Postprocessor postprocessor(wrapper);
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
                trsf_path // trsf_path si nécessaire
            );
        } catch (const std::exception& e) {
            QFAIL(qPrintable(QString("Le postprocessing a crashé : %1").arg(e.what())));
        }
    }
};

QTEST_MAIN(TestPostprocessor);
#include "test_postprocessor.moc"