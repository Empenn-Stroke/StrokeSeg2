#include <utils/niftiVolume.h>
#include "test_utils.h"
#include <QDir>
#include <QtTest>
#include <iostream>

class TestNiftiVolume : public QObject {
    Q_OBJECT

  private slots:

    void initTestCase() {
        // Create output dir
        QDir().mkpath("test_output");
    }

    void testSaveAndLoad() {
        auto vol = makeTestVolume();

        // Save
        QVERIFY_NO_THROW(QVERIFY(NiftiVolume::saveNifti(vol.file_path, vol)));

        // Load
        NiftiVolume loaded;
        QVERIFY_NO_THROW(loaded = NiftiVolume::loadNifti(vol.file_path));

        // Compare shape
        QCOMPARE(loaded.getShape(), vol.getShape());

        // Compare spacing
        QVERIFY(eigenVecEq(loaded.spacing, vol.spacing));

        // Compare some values
        QCOMPARE(loaded.data(0, 1, 2, 3), vol.data(0, 1, 2, 3));
    }

    void testToVector() {
        auto vol = makeTestVolume(2, 3, 4, 5);
        auto vec = vol.toVector();

        int expected_size = vol.data.dimension(0) * vol.data.dimension(1) * vol.data.dimension(2) *
                            vol.data.dimension(3);

        QCOMPARE(int(vec.size()), expected_size);

        // check first and last elements
        QCOMPARE(vec.front(), vol.data(0, 0, 0, 0));
        QCOMPARE(vec.back(), vol.data(1, 2, 3, 4));
    }
};

QTEST_MAIN(TestNiftiVolume)
#include "test_niftivolume.moc"
