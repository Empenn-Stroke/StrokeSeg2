#include "NiftiVolume.h"

#include <QString>
#include <cstring>
#include <stdexcept>

extern "C" {
#include <nifti1_io.h>
}

NiftiVolume NiftiVolume::loadNifti(const QString &path) {

    nifti_image *nim = nifti_image_read(path.toStdString().c_str(), 1);
    if (!nim)
        throw std::runtime_error("Failed to read NIFTI file");

    NiftiVolume vol;
    vol.file_path = path;

    // ---- Dimensions ----
    const int C = (nim->nt > 1) ? nim->nt : 1;
    const int X = nim->nx;
    const int Y = nim->ny;
    const int Z = nim->nz;

    if (X <= 0 || Y <= 0 || Z <= 0) {
        nifti_image_free(nim);
        throw std::runtime_error("Invalid NIFTI dimensions");
    }

    // ---- Spacing ----
    vol.spacing = Eigen::Vector3f(nim->dx > 0 ? nim->dx : 1.f, nim->dy > 0 ? nim->dy : 1.f,
                                  nim->dz > 0 ? nim->dz : 1.f);

    // ---- Allocate tensor ----
    vol.data = Tensor4f(C, X, Y, Z);

    const size_t voxelCount = static_cast<size_t>(C) * X * Y * Z;

    // ---- Copy / convert data ----
    const void *src = nim->data;
    if (!src) {
        nifti_image_free(nim);
        throw std::runtime_error("NIFTI data pointer is null");
    }

    switch (nim->datatype) {

    case NIFTI_TYPE_FLOAT32: {
        std::memcpy(vol.data.data(), src, voxelCount * sizeof(float));
        break;
    }

    case NIFTI_TYPE_INT16: {
        const int16_t *p = static_cast<const int16_t *>(src);
        for (size_t i = 0; i < voxelCount; ++i)
            vol.data.data()[i] = static_cast<float>(p[i]);
        break;
    }

    case NIFTI_TYPE_UINT8: {
        const uint8_t *p = static_cast<const uint8_t *>(src);
        for (size_t i = 0; i < voxelCount; ++i)
            vol.data.data()[i] = static_cast<float>(p[i]);
        break;
    }

    case NIFTI_TYPE_FLOAT64: {
        const double *p = static_cast<const double*>(src);
        for (size_t i = 0; i < voxelCount; ++i)
            vol.data.data()[i] = static_cast<float>(p[i]);
        break;
    }

    default:
        nifti_image_free(nim);
        throw std::runtime_error("Unsupported NIFTI datatype");
    }

    nifti_image_free(nim);
    return vol;
}

void NiftiVolume::saveNifti(const QString &path, const NiftiVolume &vol) {
    nifti_image *nim = nifti_simple_init_nim();

    nim->nx = vol.data.dimension(1);
    nim->ny = vol.data.dimension(2);
    nim->nz = vol.data.dimension(3);
    nim->nt = vol.data.dimension(0);

    nim->dx = vol.spacing.x();
    nim->dy = vol.spacing.y();
    nim->dz = vol.spacing.z();

    nim->datatype = NIFTI_TYPE_FLOAT32;
    nim->nbyper = sizeof(float);

    nim->dim[0] = 4;
    nim->dim[1] = nim->nx;
    nim->dim[2] = nim->ny;
    nim->dim[3] = nim->nz;
    nim->dim[4] = nim->nt;

    const size_t voxelCount = static_cast<size_t>(nim->nx) * nim->ny * nim->nz * nim->nt;

    nim->data = std::malloc(voxelCount * sizeof(float));
    if (!nim->data) {
        nifti_image_free(nim);
        throw std::runtime_error("Failed to allocate NIFTI output buffer");
    }

    std::memcpy(nim->data, vol.data.data(), voxelCount * sizeof(float));

    nifti_set_filenames(nim, path.toStdString().c_str(), 0, 1);
    nifti_image_write(nim);
    nifti_image_free(nim);
}

std::vector<float> NiftiVolume::toVector() const 
{

    return std::vector<float>(data.data(), data.data() + data.size());

}

std::vector<int64_t> NiftiVolume::getShape() const 
{
    return {static_cast<int64_t>(data.dimension(0)),
            static_cast<int64_t>(data.dimension(1)),
            static_cast<int64_t>(data.dimension(2)),
            static_cast<int64_t>(data.dimension(3))};
}