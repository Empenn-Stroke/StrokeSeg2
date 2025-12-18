#include "resampling.h"
#include <algorithm>
#include <cassert>
#include <cmath>


namespace preprocessing {

    Resampling::Resampling() {}
    Resampling::~Resampling() {}

    // ---- Shape computation ----
    std::array<int, 3> Resampling::reshape(const std::array<int, 3> &old_shape,
                                          const std::array<float, 3> &old_spacing,
                                          const std::array<float, 3> &new_spacing) {
        std::array<int, 3> new_shape;
        for (int d = 0; d < 3; ++d)
            new_shape[d] =
                static_cast<int>(std::round(old_shape[d] * old_spacing[d] / new_spacing[d]));
        return new_shape;
    }

    // ---- Détection de l'axe faible résolution ----
    bool Resampling::get_do_separate_z(const std::array<float, 3> &spacing) {
        float max_s = *std::max_element(spacing.begin(), spacing.end());
        float min_s = *std::min_element(spacing.begin(), spacing.end());
        return (min_s > 0.0f) && (max_s / min_s > separate_z_anisotropy_threshold);
    }

    int Resampling::get_single_lowres_axis(const std::array<float, 3> &spacing) {
        float max_s = *std::max_element(spacing.begin(), spacing.end());
        int axis = -1;
        for (int d = 0; d < 3; ++d) {
            if (std::abs(spacing[d] - max_s) < 1e-6f) {
                if (axis != -1)
                    return -1; // plus d'un axe max
                axis = d;
            }
        }
        return axis;
    }

    // ---- Détermination du resampling séparé ----
    std::pair<bool, int>
    Resampling::determine_separate_axis(const std::array<float, 3> &current_spacing,
                                       const std::array<float, 3> &new_spacing,
                                       bool force_separate_z, bool has_force) {
        if (has_force) {
            if (!force_separate_z)
                return {false, -1};
            int axis = get_single_lowres_axis(current_spacing);
            return (axis == -1) ? std::make_pair(false, -1) : std::make_pair(true, axis);
        }
        if (get_do_separate_z(current_spacing)) {
            int axis = get_single_lowres_axis(current_spacing);
            return (axis == -1) ? std::make_pair(false, -1) : std::make_pair(true, axis);
        }
        if (get_do_separate_z(new_spacing)) {
            int axis = get_single_lowres_axis(new_spacing);
            return (axis == -1) ? std::make_pair(false, -1) : std::make_pair(true, axis);
        }
        return {false, -1};
    }

    // ---- Interpolation helpers ----
    float Resampling::linear_interp(float v0, float v1, float t) {
        return (1.0f - t) * v0 + t * v1;
    }

    int Resampling::nearest_index(float x, int max_val) {
        int idx = std::round(x);
        return std::clamp(idx, 0, max_val - 1);
    }

    float Resampling::trilinear_interp(const Volume4D &vol, int c, float x, float y, float z) {
        int X = vol.X, Y = vol.Y, Z = vol.Z;
        int x0 = std::clamp(int(std::floor(x)), 0, X - 1);
        int x1 = std::clamp(x0 + 1, 0, X - 1);
        int y0 = std::clamp(int(std::floor(y)), 0, Y - 1);
        int y1 = std::clamp(y0 + 1, 0, Y - 1);
        int z0 = std::clamp(int(std::floor(z)), 0, Z - 1);
        int z1 = std::clamp(z0 + 1, 0, Z - 1);

        float xd = x - x0;
        float yd = y - y0;
        float zd = z - z0;

        float c00 = linear_interp(vol.at(c, x0, y0, z0), vol.at(c, x1, y0, z0), xd);
        float c01 = linear_interp(vol.at(c, x0, y0, z1), vol.at(c, x1, y0, z1), xd);
        float c10 = linear_interp(vol.at(c, x0, y1, z0), vol.at(c, x1, y1, z0), xd);
        float c11 = linear_interp(vol.at(c, x0, y1, z1), vol.at(c, x1, y1, z1), xd);

        float c0 = linear_interp(c00, c10, yd);
        float c1 = linear_interp(c01, c11, yd);

        return linear_interp(c0, c1, zd);
    }

    // ---- Resample volume 4D ----
    Volume4D Resampling::resample(const Volume4D &vol, const std::array<float, 3> &new_spacing) {
        Volume4D out;
        out.C = vol.C;
        out.spacing = new_spacing;

        std::array<int, 3> old_shape = {vol.X, vol.Y, vol.Z};
        std::array<int, 3> new_shape = reshape(old_shape, vol.spacing, new_spacing);

        out.X = new_shape[0];
        out.Y = new_shape[1];
        out.Z = new_shape[2];
        out.data.resize(out.C * out.X * out.Y * out.Z, 0.0f);

        auto [do_sep, axis] = determine_separate_axis(vol.spacing, new_spacing);

        for (int c = 0; c < vol.C; c++) {
            if (!do_sep) {
                // Full 3D resampling
                for (int i = 0; i < out.X; i++)
                    for (int j = 0; j < out.Y; j++)
                        for (int k = 0; k < out.Z; k++) {
                            float x = i * float(vol.X) / out.X;
                            float y = j * float(vol.Y) / out.Y;
                            float z = k * float(vol.Z) / out.Z;
                            if (is_seg)
                                out.at(c, i, j, k) = std::round(trilinear_interp(vol, c, x, y, z));
                            else
                                out.at(c, i, j, k) = trilinear_interp(vol, c, x, y, z);
                        }
            } else {
                for (int i = 0; i < out.X; i++)
                    for (int j = 0; j < out.Y; j++)
                        for (int k = 0; k < out.Z; k++) {
                            float x = i * float(vol.X) / out.X;
                            float y = j * float(vol.Y) / out.Y;
                            float z = k * float(vol.Z) / out.Z;
                            if (is_seg)
                                out.at(c, i, j, k) = std::round(trilinear_interp(vol, c, x, y, z));
                            else
                                out.at(c, i, j, k) = trilinear_interp(vol, c, x, y, z);
                        }
            }
        }
        return out;
    }

} // namespace preprocessing