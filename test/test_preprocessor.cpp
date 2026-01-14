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

    void initTestCase() { QDir().mkpath("test_output"); }

    void testPreprocessSingleModality() {
        // ------------------------
        // Arrange
        // ------------------------
        auto inputVol = makeTestVolume(
            /*C=*/1,
            /*X=*/4,
            /*Y=*/5,
            /*Z=*/6);

        // Write test input
        QVERIFY(NiftiVolume::saveNifti(inputVol.file_path, inputVol));

        Resampling resampler;
        MockAnimaWrapper *mockWrapper = new MockAnimaWrapper(this);
        BrainExtraction brainExtractor(mockWrapper, "");
        Preprocessor preproc(&resampler, &brainExtractor, mockWrapper);
        // ------------------------
        // Act
        // ------------------------
        PreprocessedVolume out = preproc.preprocess(inputVol.file_path,
                                                    QString(), // no flair
                                                    "test_output",
                                                    /*bet_only=*/false);

        // ------------------------
        // Assert
        // ------------------------

        // 1️⃣ Original shape preserved
        QCOMPARE(out.original_shape.x(), inputVol.data.dimension(1));
        QCOMPARE(out.original_shape.y(), inputVol.data.dimension(2));
        QCOMPARE(out.original_shape.z(), inputVol.data.dimension(3));

        // 2️⃣ Spacing preserved
        QVERIFY(eigenVecEq(out.spacing, inputVol.spacing));

        // 3️⃣ Data exists
        QVERIFY(out.data.size() > 0);

        // 4️⃣ Channel count preserved
        QCOMPARE(out.data.dimension(0), inputVol.data.dimension(0));

        // 5️⃣ Padding is consistent
        for (int i = 0; i < 3; ++i) {
            QVERIFY(out.padding[i][0] >= 0);
            QVERIFY(out.padding[i][1] >= 0);
        }

        // 6️⃣ Final size >= original size
        QCOMPARE(out.data.dimension(1),
                 inputVol.data.dimension(1) + out.padding[0][0] + out.padding[0][1]);
        QCOMPARE(out.data.dimension(2),
                 inputVol.data.dimension(2) + out.padding[1][0] + out.padding[1][1]);
        QCOMPARE(out.data.dimension(3),
                 inputVol.data.dimension(3) + out.padding[2][0] + out.padding[2][1]);
    }

    void testPreprocessKeepsNonZeroVoxel() {
        // ------------------------
        // Arrange
        // ------------------------
        auto vol = makeTestVolume(1, 8, 8, 8);
        vol.data.setZero();
        vol.data(0, 3, 4, 5) = 42.0f;

        QVERIFY(NiftiVolume::saveNifti(vol.file_path, vol));

        Resampling resampler;
        MockAnimaWrapper *mockWrapper = new MockAnimaWrapper(this);
        BrainExtraction brainExtractor(mockWrapper, "");
        Preprocessor preproc(&resampler, &brainExtractor, mockWrapper);

        // ------------------------
        // Act
        // ------------------------
        PreprocessedVolume out = preproc.preprocess(vol.file_path, QString(), "test_output", false);

        // ------------------------
        // Assert
        // ------------------------

        bool found = false;
        for (int c = 0; c < out.data.dimension(0); ++c)
            for (int x = 0; x < out.data.dimension(1); ++x)
                for (int y = 0; y < out.data.dimension(2); ++y)
                    for (int z = 0; z < out.data.dimension(3); ++z)
                        if (out.data(c, x, y, z) == 42.0f)
                            found = true;

        QVERIFY(found);
    }

    void cleanupTestCase() {
        // optional: cleanup test_output
    }
};

int main(int argc, char *argv[])
{
    // Initialise l'infrastructure Qt pour les tests (QDir, Settings, etc.)
    QCoreApplication app(argc, argv); 
    
    TestPreprocessor tc;
    return QTest::qExec(&tc, argc, argv);
}

#include "test_preprocessor.moc"
