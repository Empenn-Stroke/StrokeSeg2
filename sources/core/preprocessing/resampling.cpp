// SPDX-License-Identifier: AGPL-3.0-or-later

#include "resampling.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <QDebug>

namespace preprocessing 
{

    /**
    * @brief Computes new shape after resampling.
    *
    * Each dimension is scaled according to the ratio between original and target spacing.
    *
    * @param old_shape Original shape (X, Y, Z).
    * @param old_spacing Original spacing (sx, sy, sz).
    * @param new_spacing Target spacing (sx', sy', sz').
    * @return New shape (X', Y', Z').
    */
    Eigen::Vector3i Resampler::compute_new_shape(const Eigen::Vector3i &old_shape, const Eigen::Vector3f &old_spacing, const Eigen::Vector3f &new_spacing) const 
    {
        return (old_shape.cast<float>().array() * old_spacing.array() / new_spacing.array())
            .round()
            .cast<int>();
    }

    /**
     * @brief Checks if any axis requires separate resampling due to anisotropy.
     * @param spacing Spacing vector (sx, sy, sz).
     * @return True if separate axis resampling is needed.
     */
    bool Resampler::get_do_separate_axis(const Eigen::Vector3f &spacing) const 
    {
        return (spacing.maxCoeff() / spacing.minCoeff()) > separate_z_anisotropy_threshold;
    }

    /**
     * @brief Finds the axis with the largest spacing (lowest resolution).
     * @param spacing Spacing vector (sx, sy, sz).
     * @return Index of the axis with largest spacing, or -1 if ambiguous.
     */
    int Resampler::get_lowres_axis(const Eigen::Vector3f &spacing) const 
    {
        float max_s = spacing.maxCoeff();
        auto mask = (spacing.array() - max_s).abs() < 1e-6f;
        
        if (mask.count() != 1) 
        {
            return -1; 
        }

        int index;
        mask.cast<int>().maxCoeff(&index);
        return index;
    }

    /**
     * @brief Determines if resampling should be done separately along a single axis.
     *
     * @param current_spacing Current volume spacing.
     * @param new_spacing Target spacing.
     * @return Pair: {true if separate resampling needed, axis index}.
     */
    std::pair<bool, int>
    Resampler::determine_separate_axis(const Eigen::Vector3f &current_spacing, const Eigen::Vector3f &new_spacing) const 
    {
        if (get_do_separate_axis(current_spacing)) 
        {
            int axis = get_lowres_axis(current_spacing);
            if (axis != -1) 
            {
                return {true, axis};
            }
        }
        if (get_do_separate_axis(new_spacing))
        {
            int axis = get_lowres_axis(new_spacing);
            if (axis != -1) 
            {
                return {true, axis};
            }
        }
        return {false, -1};
    }

    /**
     * @brief Performs bilinear interpolation on a 2D slice of a 3D volume.
     *
     * @param vol Input volume.
     * @param c Channel index.
     * @param x Continuous x-coordinate.
     * @param y Continuous y-coordinate.
     * @param slice Slice index along the low-res axis.
     * @param axis Axis of low resolution (0=X,1=Y,2=Z).
     * @return Interpolated value.
     */
    float Resampler::bilinear_2d(const NiftiVolume &vol, int c, float x, float y, int slice, int axis) const 
    {
        const auto &t = vol.data;

        Eigen::Vector2i dims;
        int s_max;
        if (axis == 0) 
        {
            s_max = t.dimension(1);
            dims << t.dimension(2), t.dimension(3);
        } 
        else if (axis == 1) 
        {
            s_max = t.dimension(2);
            dims << t.dimension(1), t.dimension(3);
        } 
        else 
        {
            s_max = t.dimension(3);
            dims << t.dimension(1), t.dimension(2);
        }

        int s = std::clamp(slice, 0, s_max - 1);

        Eigen::Vector2f pos(x, y);
        Eigen::Vector2i p0 = pos.array().floor().cast<int>().cwiseMax(0).cwiseMin(dims.array() - 1);
        Eigen::Vector2i p1 = (p0.array() + 1).cwiseMin(dims.array() - 1);
        Eigen::Vector2f d = pos - p0.cast<float>();

        auto get_val = [&](int i, int j) -> float 
        {
            if (axis == 0)
            {
                return t(c, s, i, j);
            }
            if (axis == 1) 
            {
                return t(c, i, s, j);
            }
            return t(c, i, j, s);
        };

        float v00 = get_val(p0.x(), p0.y());
        float v10 = get_val(p1.x(), p0.y());
        float v01 = get_val(p0.x(), p1.y());
        float v11 = get_val(p1.x(), p1.y());

        float res_x0 = v00 + d.x() * (v10 - v00);
        float res_x1 = v01 + d.x() * (v11 - v01);
        return res_x0 + d.y() * (res_x1 - res_x0);
    }

    /**
     * @brief Nearest-neighbor interpolation along 1D axis.
     *
     * @param t Tensor data.
     * @param c Channel index.
     * @param i Coordinate along first orthogonal axis.
     * @param j Coordinate along second orthogonal axis.
     * @param pos Continuous position along axis.
     * @param axis Axis index to interpolate along.
     * @return Interpolated value.
     */
    float Resampler::nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i, int j, float pos, int axis) const 
    {
        int dim = t.dimension(axis + 1);
        int idx = std::clamp(int(std::round(pos)), 0, dim - 1);

        if (axis == 0) 
        {
            return t(c, idx, i, j);
        }
        if (axis == 1)
        {
            return t(c, i, idx, j);
        }
        return t(c, i, j, idx);
    }

    /**
     * @brief Nearest / vote interpolation for segmentation along 1D axis.
     *
     * @param t Tensor data.
     * @param c Channel index.
     * @param i Coordinate along first orthogonal axis.
     * @param j Coordinate along second orthogonal axis.
     * @param pos Continuous position along axis.
     * @param axis Axis index to interpolate along.
     * @return Interpolated value (nearest label).
     */
    float Resampler::vote_nearest_1d(const Eigen::Tensor<float, 4, Eigen::RowMajor> &t, int c, int i, int j, float pos, int axis) const 
    {
        // simple nearest for now; can implement full label voting
        return std::round(nearest_1d(t, c, i, j, pos, axis));
    }

    /**
     * @brief Resample a 4D volume to a new spacing.
     *
     * This function will perform full 3D linear interpolation if the spacing
     * is isotropic, or slice-by-slice resampling along the axis with the
     * largest spacing if anisotropy is detected.
     *
     * @param in Input NiftiVolume.
     * @param new_spacing Desired spacing in mm (sx', sy', sz').
     * @param is_segmentation True if the volume is a segmentation, using nearest/vote
     * interpolation.
     * @return Resampled NiftiVolume.
     */
    NiftiVolume Resampler::resample(const NiftiVolume &in, const Eigen::Vector3f &new_spacing, bool is_segmentation) 
    {
        NiftiVolume out;
        out.spacing = new_spacing;

        const auto &src = in.data;
        Eigen::Vector3i old_shape(src.dimension(0), src.dimension(1), src.dimension(2));
        Eigen::Vector3i new_shape = compute_new_shape(old_shape, in.spacing, new_spacing);
        int numChannels = (int)src.dimension(3);

        out.data.resize(new_shape.x(), new_shape.y(), new_shape.z(), numChannels);
        Eigen::Vector3f ratios = old_shape.cast<float>().array() / new_shape.cast<float>().array();

        Eigen::ArrayXi idxX = generate_indices(new_shape.x(), ratios.x(), old_shape.x() - 1);
        Eigen::ArrayXi idxY = generate_indices(new_shape.y(), ratios.y(), old_shape.y() - 1);
        Eigen::ArrayXi idxZ = generate_indices(new_shape.z(), ratios.z(), old_shape.z() - 1);

        auto [do_sep, axis] = determine_separate_axis(in.spacing, new_spacing);

        if (!do_sep) 
        {
            for (int c = 0; c < numChannels; ++c) 
            {
                for (int z = 0; z < new_shape.z(); ++z) 
                {
                    int src_z = idxZ[z];

                    for (int x = 0; x < new_shape.x(); ++x) 
                    {
                        for (int y = 0; y < new_shape.y(); ++y)
                        {
                            out.data(x, y, z, c) = src(idxX[x], idxY[y], src_z, c);
                        }
                    }
                }
            }

            if (is_segmentation) 
            {
                out.data = out.data.round();
            }

            return out;
        }

        int d1 = (axis + 1) % 3;
        int d2 = (axis + 2) % 3;

        Eigen::ArrayXi idxD1 = generate_indices(new_shape[d1], ratios[d1], old_shape[d1] - 1);
        Eigen::ArrayXi idxD2 = generate_indices(new_shape[d2], ratios[d2], old_shape[d2] - 1);

        for (int c = 0; c < numChannels; ++c) 
        {
            for (int s = 0; s < old_shape[axis]; ++s) 
            {
                for (int i = 0; i < new_shape[d1]; ++i) 
                {
                    for (int j = 0; j < new_shape[d2]; ++j) 
                    {
                        float val;
                        if (axis == 0)
                        {
                            val = src(s, idxD1[i], idxD2[j], c);
                        } 
                        else if (axis == 1) 
                        {
                            val = src(idxD1[i], s, idxD2[j], c);
                        }
                        else 
                        {
                            val = src(idxD1[i], idxD2[j], s, c);
                        }

                        for (int ns = 0; ns < new_shape[axis]; ++ns) 
                        {
                            if (idxZ[ns] == s) 
                            {
                                if (axis == 0) 
                                {
                                    out.data(ns, i, j, c) = val;
                                }
                                else if (axis == 1) 
                                {
                                    out.data(i, ns, j, c) = val;
                                }
                                else
                                {
                                    out.data(i, j, ns, c) = val;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (is_segmentation) 
        {
            out.data = out.data.round();
        }
        return out;
    }

    Eigen::ArrayXi Resampler::generate_indices(int new_dim, float ratio, int max_val) 
    {
        return (Eigen::ArrayXf::LinSpaced(new_dim, 0, new_dim - 1) * ratio)
            .round()
            .cast<int>()
            .cwiseMin(max_val)
            .cwiseMax(0);
    }

} // namespace preprocessing
