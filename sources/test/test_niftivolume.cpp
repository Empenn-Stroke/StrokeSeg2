#include "test_utils.h"
#include <QDir>
#include <QtTest>
#include <iostream>
#include <utils/niftiVolume.h>

class TestNiftiVolume : public QObject {
    Q_OBJECT

  private slots:

    void initTestCase() { QDir().mkpath("test_output"); }

    void testSaveAndLoad() {
        auto vol = makeTestVolume();

        // Sauvegarde
        QVERIFY(NiftiVolume::saveNifti(vol.file_path, vol));

        // Chargement
        NiftiVolume loaded = NiftiVolume::loadNifti(vol.file_path);

        // Shape
        QCOMPARE(loaded.getShape(), vol.getShape());

        // Spacing
        QVERIFY(eigenVecEq(loaded.spacing, vol.spacing));

        // Vérifie quelques valeurs
        QCOMPARE(loaded.data(0, 0, 0, 0), vol.data(0, 0, 0, 0));
        QCOMPARE(loaded.data(0, 1, 1, 1), vol.data(0, 1, 1, 1));
    }

    void testToVector() {
        auto vol = makeTestVolume(2, 2, 2, 2);
        auto vec = vol.toVector();

        int expected_size = vol.data.dimension(0) * vol.data.dimension(1) * vol.data.dimension(2) *
                            vol.data.dimension(3);
        QCOMPARE(int(vec.size()), expected_size);

        QCOMPARE(vec.front(), vol.data(0, 0, 0, 0));
        QCOMPARE(vec.back(), vol.data(1, 1, 1, 1));
    }
};

QTEST_MAIN(TestNiftiVolume)
#include "test_niftivolume.moc"
