#include <QtTest/QTest>
#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include <preprocessing/resampling.h>
#include <utils/niftiVolume.h>

using preprocessing::Resampling;

class TestResampling : public QObject {
    Q_OBJECT

private:
    static NiftiVolume makeRampVolume(int C, int X, int Y, int Z,
                                      const Eigen::Vector3f &spacing) {
        NiftiVolume v;
        v.spacing = spacing;
        v.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(C, X, Y, Z);

        for (int c = 0; c < C; ++c)
            for (int x = 0; x < X; ++x)
                for (int y = 0; y < Y; ++y)
                    for (int z = 0; z < Z; ++z)
                        v.data(c, x, y, z) = float(x + y + z);

        return v;
    }

    static NiftiVolume makeLabelVolume(int X, int Y, int Z,
                                       const Eigen::Vector3f &spacing) {
        NiftiVolume v;
        v.spacing = spacing;
        v.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(1, X, Y, Z);

        for (int x = 0; x < X; ++x)
            for (int y = 0; y < Y; ++y)
                for (int z = 0; z < Z; ++z)
                    v.data(0, x, y, z) = (z < Z / 2) ? 1.f : 2.f;

        return v;
    }

private slots:

    void test_IsotropicShape() {
        Resampling r;

        auto in = makeRampVolume(
            1, 10, 10, 10,
            Eigen::Vector3f(1.f, 1.f, 1.f)
        );

        auto out = r.resample(in, Eigen::Vector3f(0.5f, 0.5f, 0.5f));

        QCOMPARE(out.data.dimension(1), 20);
        QCOMPARE(out.data.dimension(2), 20);
        QCOMPARE(out.data.dimension(3), 20);

        QCOMPARE(out.spacing.x(), 0.5f);
        QCOMPARE(out.spacing.y(), 0.5f);
        QCOMPARE(out.spacing.z(), 0.5f);
    }

    void test_IsotropicPreservesConstant() {
        Resampling r;

        NiftiVolume v;
        v.spacing = Eigen::Vector3f(1, 1, 1);
        v.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(1, 8, 8, 8);
        v.data.setConstant(42.f);

        auto out = r.resample(v, Eigen::Vector3f(0.5f, 0.5f, 0.5f));

        for (int x = 0; x < out.data.dimension(1); ++x)
            for (int y = 0; y < out.data.dimension(2); ++y)
                for (int z = 0; z < out.data.dimension(3); ++z)
                    QCOMPARE(out.data(0, x, y, z), 42.f);
    }

    void test_AnisotropicZ() {
        Resampling r;

        auto in = makeRampVolume(
            1, 20, 20, 5,
            Eigen::Vector3f(1.f, 1.f, 5.f)
        );

        auto out = r.resample(in, Eigen::Vector3f(1.f, 1.f, 1.f));

        QCOMPARE(out.data.dimension(1), 20);
        QCOMPARE(out.data.dimension(2), 20);
        QCOMPARE(out.data.dimension(3), 25); // 5 * 5 / 1
    }

    void test_SegmentationPreservesLabels() {
        Resampling r;

        auto in = makeLabelVolume(
            10, 10, 4,
            Eigen::Vector3f(1.f, 1.f, 4.f)
        );

        auto out = r.resample(in, Eigen::Vector3f(1.f, 1.f, 1.f), true);

        for (int x = 0; x < out.data.dimension(1); ++x)
            for (int y = 0; y < out.data.dimension(2); ++y)
                for (int z = 0; z < out.data.dimension(3); ++z) {
                    float v = out.data(0, x, y, z);
                    QVERIFY(v == 1.f || v == 2.f);
                }
    }

    void test_NoCrashOnIdentity() {
        Resampling r;

        auto in = makeRampVolume(
            1, 16, 16, 16,
            Eigen::Vector3f(1.f, 1.f, 1.f)
        );

        auto out = r.resample(in, Eigen::Vector3f(1.f, 1.f, 1.f));

        QCOMPARE(out.data.dimension(1), 16);
        QCOMPARE(out.data.dimension(2), 16);
        QCOMPARE(out.data.dimension(3), 16);
    }
};

QTEST_MAIN(TestResampling)
#include "test_resampling.moc"