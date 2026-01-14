#pragma once

#include <QString>
#include <QtTest>
#include <exception>

/**
 * @brief Verify that a statement does not throw any exception.
 *
 * If an exception is thrown, the test fails and prints the exception message.
 *
 * Usage:
 *   QVERIFY_NO_THROW( foo() );
 *   QVERIFY_NO_THROW( obj.method(arg) );
 */
#define QVERIFY_NO_THROW(stmt)                                                                     \
    do {                                                                                           \
        try {                                                                                      \
            stmt;                                                                                  \
        } catch (const std::exception &e) {                                                        \
            QFAIL(qPrintable(QString("Exception thrown: ") + e.what()));                           \
        } catch (...) {                                                                            \
            QFAIL("Unknown exception thrown");                                                     \
        }                                                                                          \
    } while (false)

/**
 * @brief Verify that a statement throws a specific exception type.
 *
 * Wrapper around QVERIFY_EXCEPTION_THROWN with better readability.
 *
 * Usage:
 *   QVERIFY_THROW(std::runtime_error, foo());
 */
#define QVERIFY_THROW(exception_type, stmt) QVERIFY_EXCEPTION_THROWN(stmt, exception_type)

/**
 * @brief Verify two floating point values are approximately equal.
 *
 * Usage:
 *   QVERIFY_FLOAT_EQ(a, b);
 */
#define QVERIFY_FLOAT_EQ(a, b) QVERIFY(qFuzzyCompare(static_cast<float>(a), static_cast<float>(b)))

/**
 * @brief Verify two Eigen vectors are approximately equal.
 *
 * Works for Eigen::Vector3f, Vector4f, etc.
 *
 * Usage:
 *   QVERIFY_EIGEN_VEC_EQ(v1, v2);
 */
#define QVERIFY_EIGEN_VEC_EQ(v1, v2)                                                               \
    do {                                                                                           \
        QVERIFY((v1).isApprox((v2)));                                                              \
    } while (false)

#pragma once
#include <Eigen/Core>
#include <array>
#include <utils/niftiVolume.h>

// Génère un petit volume 4D pour les tests
inline NiftiVolume makeTestVolume(int C = 1, int X = 2, int Y = 2, int Z = 2) {
    NiftiVolume vol;

    vol.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(C, X, Y, Z);
    vol.spacing = {1.0f, 1.0f, 1.0f};

    // Remplir avec des valeurs simples
    int val = 1;
    for (int c = 0; c < C; ++c)
        for (int x = 0; x < X; ++x)
            for (int y = 0; y < Y; ++y)
                for (int z = 0; z < Z; ++z)
                    vol.data(c, x, y, z) = static_cast<float>(val++);

    // Chemin de sauvegarde pour le test
    vol.file_path = "test_output/dummy.nii";

    return vol;
}

// Comparaison de vecteurs Eigen::Vector3f
inline bool eigenVecEq(const Eigen::Vector3f &a, const Eigen::Vector3f &b, float eps = 1e-5f) {
    return (a - b).cwiseAbs().maxCoeff() < eps;
}
