#include <QDir>
#include <QtTest>

#include <preprocessing/preprocessor.h>
#include <utils/niftiVolume.h>
#include <utils/path.h>

#include "test_utils.h"
#include "mockanimawrapper.h"

using namespace preprocessing;

class TestPreprocessor : public QObject {
    Q_OBJECT

  private slots:

    void initTestCase() {  }

    //void testPreprocessSingleModality() {

    //    // Utiliser un dossier temporaire système pour éviter les problèmes de droits/chemins longs
    //    QString testOutDir = QDir::tempPath() + "/strokeseg_unit_test";
    //    QDir().mkpath(testOutDir);

    //    auto &config = ConfigManager::instance();
    //    config.set("save_preproc", true);
    //    config.set("keep_MNI", true);

    //    // Créer un volume de test dans le dossier temporaire
    //    auto inputVol = makeTestVolume(1, 64, 64, 64);
    //    inputVol.file_path = testOutDir + "/input_dummy.nii.gz"; // Chemin explicite
    //    inputVol.spacing = Eigen::Vector3f(2.0f, 2.0f, 2.0f);

    //    QVERIFY(NiftiVolume::saveNifti(inputVol.file_path, inputVol));

    //    Resampling resampler;
    //    MockAnimaWrapper *mockWrapper = new MockAnimaWrapper(this);
    //    BrainExtraction brainExtractor(mockWrapper, "");
    //    Preprocessor preproc(&resampler, &brainExtractor, mockWrapper);

    //    // Exécuter
    //    PreprocessedVolume out;
    //    try {
    //        out = preproc.preprocess(inputVol.file_path, QString(), testOutDir, false);
    //    } catch (const std::exception &e) {
    //        QFAIL(qPrintable(QString("Exception dans preprocess: %1").arg(e.what())));
    //    }

    //    QFileInfo checkFile(out.MNI_base_image);
    //    qDebug() << "Fichier MNI généré :" << out.MNI_base_image;
    //    qDebug() << "Existe ?" << checkFile.exists();
    //    qDebug() << "Taille :" << checkFile.size() << "octets";

    //    if (checkFile.size() < 1000) {
    //        QFAIL("Le fichier généré est trop petit ou vide !");
    //    }

    //    QCOMPARE(out.original_shape.x(), 64);
    //    QCOMPARE(out.original_shape.y(), 64);
    //    QCOMPARE(out.original_shape.z(), 64);

    //    QVERIFY(eigenVecEq(out.spacing, Eigen::Vector3f(1.0f, 1.0f, 1.0f)));

    //    QVERIFY(out.data.size() > 0);
    //    QCOMPARE(out.data.dimension(0), 1);

    //    for (int i = 0; i < 3; ++i) {
    //        int finalDim = out.data.dimension(i + 1);
    //        int padBefore = out.padding[i][0];
    //        int padAfter = out.padding[i][1];
    //        int sizeBeforePadding = finalDim - (padBefore + padAfter);

    //        QVERIFY(sizeBeforePadding > 0);

    //        QVERIFY(finalDim >= 128);
    //    }

    //    float mean = 0;
    //    float *ptr = out.data.data();
    //    for (int i = 0; i < out.data.size(); ++i)
    //        mean += ptr[i];
    //    mean /= out.data.size();

    //    QVERIFY(std::abs(mean) < 0.1f);

    //    QVERIFY(!out.MNI_base_image.isEmpty());
    //    QVERIFY(QFile::exists(out.MNI_base_image));
    //}

    //void testPreprocessKeepsNonZeroVoxel() {
    //    QString absolutePath = QDir::currentPath() + "/test_signal.nii.gz";

    //    auto vol = makeTestVolume(1, 24, 24, 24);
    //    vol.file_path = absolutePath; // On force le chemin
    //    vol.data.setZero();

    //    // Signal très large pour être immanquable
    //    for (int x = 5; x < 19; ++x)
    //        for (int y = 5; y < 19; ++y)
    //            for (int z = 5; z < 19; ++z)
    //                vol.data(0, x, y, z) = 100.0f;

    //    QVERIFY(NiftiVolume::saveNifti(vol.file_path, vol));

    //    Resampling resampler;
    //    MockAnimaWrapper *mockWrapper = new MockAnimaWrapper(this);
    //    BrainExtraction brainExtractor(mockWrapper, "");
    //    Preprocessor preproc(&resampler, &brainExtractor, mockWrapper);

    //    PreprocessedVolume out = preproc.preprocess(vol.file_path, QString(), "test_output", false);

    //    bool found_signal = false;
    //    for (int i = 0; i < out.data.size(); ++i) {
    //        // On cherche une valeur absolue > 0.01
    //        // Même après Z-score, les différences entre 110.0 et 114.0 survivront
    //        if (std::abs(out.data.data()[i]) > 0.01f) {
    //            found_signal = true;
    //            break;
    //        }
    //    }
    //    qDebug() << "Dimensions finales :" << out.data.dimension(1) << "x" << out.data.dimension(2);
    //    qDebug() << "Valeur au centre :" << out.data(0, 64, 64, 64);
    //    QVERIFY(found_signal);
    //}

    //void testPreprocessMultimodal() {

    //    try {
    //        auto &config = ConfigManager::instance();
    //        config.set("save_preproc", true);

    //        // 1. Création des volumes T1 et FLAIR
    //        QString t1_path = QDir::currentPath() + "/test_t1.nii.gz";
    //        QString flair_path = QDir::currentPath() + "/test_flair.nii.gz";

    //        auto t1_vol = makeTestVolume(1, 30, 30, 30);
    //        t1_vol.file_path = t1_path;
    //        t1_vol.data.setConstant(100.0f); // T1 uniforme
    //        QVERIFY(NiftiVolume::saveNifti(t1_path, t1_vol));

    //        auto flair_vol = makeTestVolume(1, 30, 30, 30);
    //        flair_vol.file_path = flair_path;
    //        flair_vol.data.setConstant(200.0f); // FLAIR uniforme mais différent
    //        QVERIFY(NiftiVolume::saveNifti(flair_path, flair_vol));

    //        // 2. Setup du Preprocessor
    //        Resampling resampler;
    //        MockAnimaWrapper *mockWrapper = new MockAnimaWrapper(this);
    //        BrainExtraction brainExtractor(mockWrapper, "");
    //        Preprocessor preproc(&resampler, &brainExtractor, mockWrapper);

    //        // 3. Exécution
    //        qDebug() << "Appel de preprocess multimodal...";
    //        PreprocessedVolume out = preproc.preprocess(t1_path, flair_path, "test_output", false);

    //        qDebug() << "Vérification des dimensions finales...";
    //        qDebug() << "Canaux:" << out.data.dimension(0);
    //        qDebug() << "X:" << out.data.dimension(1) << "Y:" << out.data.dimension(2)
    //                 << "Z:" << out.data.dimension(3);

    //        // 4. Vérifications
    //        // On attend 2 canaux (C=2)
    //        QCOMPARE(out.data.dimension(0), 2);

    //        // On vérifie que les dimensions sont cohérentes (128x128x128)
    //        QCOMPARE(out.data.dimension(1), 128);
    //    } 
    //    catch (const std::exception &e) {
    //        qFatal("EXCEPTION CAPTURÉE DANS LE TEST : %s", e.what());
    //    } 
    //    catch (...) {
    //        qFatal("EXCEPTION INCONNUE CAPTURÉE DANS LE TEST");
    //    }
    //}

    void testIntegrationT1Pipeline() {
        // 1. Chemins
        QString inputPath = QDir(base_dir).filePath("test/test_data/sub-r001s002-T1w.nii.gz");
        QString referencePath =
            QDir(base_dir).filePath("test/test_data/sub-r001s002_T1w_ss_N4_MNI.nii.gz");
        QString outputDir = QCoreApplication::applicationDirPath() + "/output_data";
        QDir().mkpath(outputDir);

        // 2. Initialisation
        Resampling resampler;
        AnimaWrapper *realWrapper = new AnimaWrapper(this);
        QString atlasImage = atlas_dir + "/Reference_T1.nrrd";
        BrainExtraction brainExtractor(realWrapper, atlasImage);
        Preprocessor preproc(&resampler, &brainExtractor, realWrapper);

        // 3. Exécution
        qDebug() << "Lancement du pipeline d'integration sur :" << inputPath;
        PreprocessedVolume result;
        try {
            result = preproc.preprocess(inputPath, "", outputDir, false);
        } catch (const std::exception &e) {
            QFAIL(qPrintable(QString("Le pipeline a crashe : %1").arg(e.what())));
        }

        // 4. Préparation du volume produit (on utilise le tenseur final result.data)
        QString finalPath = outputDir + "/final_processed_result.nii.gz";
        NiftiVolume volToSave;

        //if (result.data.dimension(0) > 2) {
        //    qDebug() << "WARNING: Redressement des axes détecté (C > 1)";
        //    volToSave.data = result.data.shuffle(Eigen::array<int, 4>{1, 2, 3, 0});
        //} else {
        //    volToSave.data = result.data;
        //}

        volToSave.data = result.data;
        volToSave.spacing = result.spacing;
        NiftiVolume::saveNifti(finalPath, volToSave);

        // Chargement pour comparaison
        NiftiVolume volProduced = NiftiVolume::loadNifti(finalPath);
        NiftiVolume volExpected = NiftiVolume::loadNifti(referencePath);

        // 5. Diagnostic Dimensions & Spacing
        qDebug() << "--- DIAGNOSTIC ---";
        qDebug() << "Dimensions Produit  :" << volProduced.data.dimension(0) << "x"
                 << volProduced.data.dimension(1) << "x" << volProduced.data.dimension(2);
        qDebug() << "Dimensions Expected :" << volExpected.data.dimension(0) << "x"
                 << volExpected.data.dimension(1) << "x" << volExpected.data.dimension(2);

        /*QCOMPARE(volProduced.data.dimension(1), volExpected.data.dimension(1));
        QCOMPARE(volProduced.data.dimension(2), volExpected.data.dimension(2));
        QCOMPARE(volProduced.data.dimension(3), volExpected.data.dimension(3));*/

        // 6. Calcul de la MSE (Mean Squared Error)
        // Dans votre test_preprocessor.cpp
        double ssd = 0.0;
        double sum_prod = 0.0;
        double sum_p = 0.0, sum_e = 0.0;
        double sum_p2 = 0.0, sum_e2 = 0.0;
        int count = volProduced.data.size();

        for (int i = 0; i < count; ++i) {
            float p = volProduced.data.data()[i];
            float e = volExpected.data.data()[i];

            double diff = p - e;
            ssd += (diff * diff);

            // Pour le calcul de corrélation de Pearson
            sum_prod += (p * e);
            sum_p += p;
            sum_e += e;
            sum_p2 += (p * p);
            sum_e2 += (e * e);
        }

        double mse = ssd / count;
        double correlation =
            (count * sum_prod - sum_p * sum_e) /
            std::sqrt((count * sum_p2 - sum_p * sum_p) * (count * sum_e2 - sum_e * sum_e));

        qDebug() << "MSE :" << mse;
        qDebug() << "Corrélation (Pearson) :" << correlation;

        // Un recalage médical est considéré comme excellent si Correlation > 0.95
        QVERIFY2(correlation > 0.98, "Les images sont spatialement identiques mais les intensites "
                                     "ne sont pas assez correlees.");
    }
};

QTEST_MAIN(TestPreprocessor);

#include "test_preprocessor.moc"
