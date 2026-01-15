#pragma once

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include <array>
#include <string>
#include <vector>

struct PreprocessedVolume {
    /**
     * @brief Volume data (C, X, Y, Z)
     */
    Eigen::Tensor<float, 4, Eigen::RowMajor> data;

    /**
     * @brief Affine transformation matrix (voxel → world)
     */
    Eigen::Matrix4f affine = Eigen::Matrix4f::Identity();

    /**
     * @brief Original image shape before preprocessing (X, Y, Z)
     */
    Eigen::Vector3i original_shape;

    /**
     * @brief Path to the deformation / transformation file
     */
    QString trsf_path;

    /**
     * @brief Voxel spacing (sx, sy, sz)
     */
    Eigen::Vector3f spacing;

    /**
     * @brief Padding applied on each axis: {{x0, x1}, {y0, y1}, {z0, z1}}
     */
    std::array<std::array<int, 2>, 3> padding{{{0, 0}, {0, 0}, {0, 0}}};

    /**
     * @brief Reference MNI image used for registration
     */
    QString MNI_base_image;
};
