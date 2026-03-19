#include "resampling.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <QDebug>

namespace preprocessing {

    // ------------------------------------------------------------
    // Shape
    // ------------------------------------------------------------
    Eigen::Vector3i Resampling::compute_new_shape(const Eigen::Vector3i &old_shape,
                                                  const Eigen::Vector3f &old_spacing,
                                                  const Eigen::Vector3f &new_spacing) const {
        Eigen::Vector3i s;
        for (int i = 0; i < 3; ++i)
            s[i] = int(std::round(old_shape[i] * old_spacing[i] / new_spacing[i]));
        return s;
    }

    // ------------------------------------------------------------
    // Anisotropy helpers
    // ------------------------------------------------------------
    bool Resampling::get_do_separate_axis(const Eigen::Vector3f &spacing) const {
        return (spacing.maxCoeff() / spacing.minCoeff()) > separate_z_anisotropy_threshold;
    }

    int Resampling::get_lowres_axis(const Eigen::Vector3f &spacing) const {
        float max_s = spacing.maxCoeff();
        int axis = -1;
        for (int i = 0; i < 3; ++i) {
            if (std::abs(spacing[i] - max_s) < 1e-6f) {
                if (axis != -1)
                    return -1; // ambiguous
                axis = i;
            }
        }
        return axis;
    }

    std::pair<bool, int>
    Resampling::determine_separate_axis(const Eigen::Vector3f &current_spacing,
                                        const Eigen::Vector3f &new_spacing) const {
        if (get_do_separate_axis(current_spacing)) {
            int axis = get_lowres_axis(current_spacing);
            if (axis != -1)
                return {true, axis};
        }
        if (get_do_separate_axis(new_spacing)) {
            int axis = get_lowres_axis(new_spacing);
            if (axis != -1)
                return {true, axis};
        }
        return {false, -1};
    }

    // ------------------------------------------------------------
    // Interpolation helpers
    // ------------------------------------------------------------
    float Resampling::bilinear_2d(const NiftiVolume &vol, int c, float x, float y, int slice,
                                  int axis) const {
        const auto &t = vol.data;
        int dim0, dim1, dim_slice;

        if (axis == 0) {
            dim_slice = t.dimension(1);
            dim0 = t.dimension(2);
            dim1 = t.dimension(3);
        } else if (axis == 1) {
            dim_slice = t.dimension(2);
            dim0 = t.dimension(1);
            dim1 = t.dimension(3);
        } else {
            dim_slice = t.dimension(3);
            dim0 = t.dimension(1);
            dim1 = t.dimension(2);
        }

        int s = std::clamp(slice, 0, dim_slice - 1);

        int x0 = std::clamp(int(std::floor(x)), 0, dim0 - 1);
        int x1 = std::min(x0 + 1, dim0 - 1);
        int y0 = std::clamp(int(std::floor(y)), 0, dim1 - 1);
        int y1 = std::min(y0 + 1, dim1 - 1);

        float dx = x - x0;
        float dy = y - y0;

        auto access = [&](int i0, int j0) {
            if (axis == 0)
                return t(c, s, i0, j0);
            if (axis == 1)
                return t(c, i0, s, j0);
            return t(c, i0, j0, s);
        };

        return std::lerp(std::lerp(access(x0, y0), access(x1, y0), dx),
                         std::lerp(access(x0, y1), access(x1, y1), dx), dy);
    }

    float Resampling::nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i,
                                 int j, float pos, int axis) const {
        int dim = t.dimension(axis + 1);
        int idx = std::clamp(int(std::round(pos)), 0, dim - 1);

        if (axis == 0)
            return t(c, idx, i, j);
        if (axis == 1)
            return t(c, i, idx, j);
        return t(c, i, j, idx);
    }

    // vote nearest for segmentation
    float Resampling::vote_nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c,
                                      int i, int j, float pos, int axis) const {
        // simple nearest for now; can implement full label voting
        return std::round(nearest_1d(t, c, i, j, pos, axis));
    }

    // ------------------------------------------------------------
    // Main resampling
    // ------------------------------------------------------------
    NiftiVolume Resampling::resample(const NiftiVolume &in, const Eigen::Vector3f &new_spacing,
                                     bool is_segmentation) {

        qDebug() << "RESAMPLE START - Input Dims:" << in.data.dimension(0) << "x" // C
                 << in.data.dimension(1) << "x"                                   // X
                 << in.data.dimension(2) << "x"                                   // Y
                 << in.data.dimension(3);                                         // Z

        NiftiVolume out;
        out.spacing = new_spacing;

        const auto &src = in.data;
        Eigen::Vector3i old_shape(src.dimension(1), src.dimension(2), src.dimension(3));
        Eigen::Vector3i new_shape = compute_new_shape(old_shape, in.spacing, new_spacing);

        out.data = Eigen::Tensor<float, 4, Eigen::ColMajor>(src.dimension(0), new_shape.x(),
                                                            new_shape.y(), new_shape.z());

        qDebug() << "Allocating output volume:" << new_shape.x() << "x" << new_shape.y() << "x"
                 << new_shape.z();

        auto [do_sep, axis] = determine_separate_axis(in.spacing, new_spacing);

        if (!do_sep) {
            // isotropic / full resampling
            for (int c = 0; c < src.dimension(0); ++c)
                for (int x = 0; x < new_shape.x(); ++x)
                    for (int y = 0; y < new_shape.y(); ++y)
                        for (int z = 0; z < new_shape.z(); ++z) {
                            float ix = x * float(old_shape.x()) / new_shape.x();
                            float iy = y * float(old_shape.y()) / new_shape.y();
                            float iz = z * float(old_shape.z()) / new_shape.z();
                            // trilinear approx: bilinear xy + nearest z
                            float v = bilinear_2d(in, c, ix, iy, int(std::round(iz)), 2);
                            out.data(c, x, y, z) = is_segmentation ? std::round(v) : v;
                        }
            return out;
        }

        // Inside Resampling::resample, anisotropic branch:
        int d1_idx = (axis + 1) % 3;
        int d2_idx = (axis + 2) % 3;

        // Intermediate tensor: (C, NewDim1, NewDim2, OldDimAxis)
        Eigen::Tensor<float, 4, Eigen::RowMajor> tmp(src.dimension(0), new_shape[d1_idx],
                                                     new_shape[d2_idx], old_shape[axis]);

        // Step 1: Resize 2D slices
        for (int c = 0; c < src.dimension(0); ++c)
            for (int s = 0; s < old_shape[axis]; ++s)
                for (int i = 0; i < new_shape[d1_idx]; ++i)
                    for (int j = 0; j < new_shape[d2_idx]; ++j) {
                        float ix = i * float(old_shape[d1_idx]) / new_shape[d1_idx];
                        float iy = j * float(old_shape[d2_idx]) / new_shape[d2_idx];
                        float v = bilinear_2d(in, c, ix, iy, s, axis);
                        // For segmentations, round immediately to prevent label bleeding
                        tmp(c, i, j, s) = is_segmentation ? std::round(v) : v;
                    }

        // step2: resize along low-res axis
        for (int c = 0; c < src.dimension(0); ++c)
            for (int i = 0; i < new_shape[d1_idx]; ++i)
                for (int j = 0; j < new_shape[d2_idx]; ++j)
                    for (int s = 0; s < new_shape[axis]; ++s) {
                        float pos = s * float(old_shape[axis]) / new_shape[axis];

                        // Use a direct nearest lookup on the last dimension of tmp
                        int idx = std::clamp(int(std::round(pos)), 0, old_shape[axis] - 1);
                        float v = tmp(c, i, j, idx);

                        // Map back to output volume
                        if (axis == 0)
                            out.data(c, s, i, j) = v;
                        else if (axis == 1)
                            out.data(c, i, s, j) = v;
                        else
                            out.data(c, i, j, s) = v;
                    }

        return out;
    }

} // namespace preprocessing
