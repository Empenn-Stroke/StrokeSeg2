#include "preprocessor.h"

preprocessing::Preprocessor::Preprocessor() {}

preprocessing::Preprocessor::~Preprocessor() {}

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

