#include "preprocessor.h"
#include <cassert>

Volume4D preprocessing::Preprocessor::preprocess(const Volume4D &vol,
                                                 const std::array<float, 3> &target_spacing) {
    return Volume4D();
}

Volume4D preprocessing::Preprocessor::loadVolume(const QString &path) {
    // --- Load file ---
    nifti_image *nim = nifti_image_read(path.toStdString().c_str(), 1);
    if (!nim)
        throw std::runtime_error("Failed to read NIFTI file");

    Volume4D vol{};

    // --- Dimensions ---
    vol.C = (nim->nt > 1) ? nim->nt : 1;
    vol.X = nim->nx;
    vol.Y = nim->ny;
    vol.Z = nim->nz;

    // --- Spacing ---
    vol.spacing = {nim->dx > 0 ? nim->dx : 1.f, nim->dy > 0 ? nim->dy : 1.f,
                   nim->dz > 0 ? nim->dz : 1.f};

    const size_t voxelCount = static_cast<size_t>(vol.C) * vol.X * vol.Y * vol.Z;
    vol.data.resize(voxelCount);

    // --- Copy data and convert ---
    const void *src = nim->data;

    switch (nim->datatype) {
    case NIFTI_TYPE_FLOAT32: {
        std::memcpy(vol.data.data(), src, voxelCount * sizeof(float));
        break;
    }
    case NIFTI_TYPE_INT16: {
        const short *p = static_cast<const short *>(src);
        for (size_t i = 0; i < voxelCount; ++i)
            vol.data[i] = static_cast<float>(p[i]);
        break;
    }
    case NIFTI_TYPE_UINT8: {
        const unsigned char *p = static_cast<const unsigned char *>(src);
        for (size_t i = 0; i < voxelCount; ++i)
            vol.data[i] = static_cast<float>(p[i]);
        break;
    }
    default:
        nifti_image_free(nim);
        throw std::runtime_error("Unsupported NIFTI datatype");
    }

    nifti_image_free(nim);
    return vol;
}

void preprocessing::Preprocessor::zScoreNormalize(Volume4D &data, const Volume4D *seg) {
    const int C = data.C;
    const int X = data.X;
    const int Y = data.Y;
    const int Z = data.Z;

    double sum = 0.0;
    double sq_sum = 0.0;
    size_t count = 0;

    
    if (count == 0)
        return;

    const double mean = data.mean();
    const double var = data.variance();
    const double std = std::sqrt(std::max(var, 1e-8));

    // Normalize
    for (int c = 0; c < C; ++c) {
        for (int x = 0; x < X; ++x) {
            for (int y = 0; y < Y; ++y) {
                for (int z = 0; z < Z; ++z) {

                    if (seg && seg->at(0, x, y, z) < 0)
                        continue;

                    data.at(c, x, y, z) = static_cast<float>((data.at(c, x, y, z) - mean) / std);
                }
            }
        }
    }
}

std::vector<bool> preprocessing::Preprocessor::computeNonZeroMask(const Volume4D &data) {

    assert(data.C > 0);

    const int C = data.C;
    const int X = data.X;
    const int Y = data.Y;
    const int Z = data.Z;

    std::vector<bool> mask(X * Y * Z, false);

    for (int x = 0; x < X; ++x) {
        for (int y = 0; y < Y; ++y) {
            for (int z = 0; z < Z; ++z) {
                bool nonzero = false;
                for (int c = 0; c < C; ++c) {
                    if (data.at(c, x, y, z) != 0.0f) {
                        nonzero = true;
                        break;
                    }
                }
                mask[x * Y * Z + y * Z + z] = nonzero;
            }
        }
    }

    return mask;
}

Volume4D preprocessing::Preprocessor::cropToNonZero(const Volume4D &data, 
    Volume4D* seg,
    int nonzero_label,
    std::array<std::array<int, 2>, 3>* bbox_out) 
{
    // compute non-zero mask
    auto mask = computeNonZeroMask(data);
    const int X = data.X;
    const int Y = data.Y;
    const int Z = data.Z;

    // compute bounding box if not provided
    std::array<std::array<int, 2>, 3> bbox;
    if (bbox_out) {
        bbox = *bbox_out;
    } else {
        int x_min = X - 1, x_max = 0;
        int y_min = Y - 1, y_max = 0;
        int z_min = Z - 1, z_max = 0;
        bool found = false;

        for (int x = 0; x < X; ++x) {
            for (int y = 0; y < Y; ++y) {
                for (int z = 0; z < Z; ++z) {
                    if (mask[x * Y * Z + y * Z + z]) {
                        x_min = std::min(x_min, x);
                        x_max = std::max(x_max, x);
                        y_min = std::min(y_min, y);
                        y_max = std::max(y_max, y);
                        z_min = std::min(z_min, z);
                        z_max = std::max(z_max, z);
                        found = true;
                    }
                }
            }
        }
        if (!found)
            throw std::runtime_error("All-zero volume, cannot crop");
        bbox = {{{x_min, x_max}, {y_min, y_max}, {z_min, z_max}}};
    }

    if (bbox_out)
        *bbox_out = bbox;

    // create new cropped volume
    Volume4D cropped;
    cropped.C = data.C;
    cropped.X = bbox[0][1] - bbox[0][0] + 1;
    cropped.Y = bbox[1][1] - bbox[1][0] + 1;
    cropped.Z = bbox[2][1] - bbox[2][0] + 1;
    cropped.spacing = data.spacing;
    cropped.data.resize(cropped.C * cropped.X * cropped.Y * cropped.Z);

    // copy data
    for (int c = 0; c < data.C; ++c) {
        for (int x = 0; x < cropped.X; ++x) {
            for (int y = 0; y < cropped.Y; ++y) {
                for (int z = 0; z < cropped.Z; ++z) {
                    cropped.at(c, x, y, z) =
                        data.at(c, x + bbox[0][0], y + bbox[1][0], z + bbox[2][0]);
                }
            }
        }
    }

    // crop seg if provided
    if (seg) {
        for (int c = 0; c < seg->C; ++c) {
            for (int x = 0; x < cropped.X; ++x) {
                for (int y = 0; y < cropped.Y; ++y) {
                    for (int z = 0; z < cropped.Z; ++z) {
                        float val = seg->at(c, x + bbox[0][0], y + bbox[1][0], z + bbox[2][0]);
                        if (val == 0 && !mask[(x + bbox[0][0]) * Y * Z + (y + bbox[1][0]) * Z +
                                              (z + bbox[2][0])])
                            val = nonzero_label;
                        cropped.at(c, x, y, z) = val;
                    }
                }
            }
        }
    }

    return cropped;
}

