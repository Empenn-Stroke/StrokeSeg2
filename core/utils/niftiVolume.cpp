#include "NiftiVolume.h"
#include <QDir>
#include <QString>
#include <cstring>
#include <stdexcept>
#include <QDebug>

extern "C" {
#include <nifti1_io.h>
}

NiftiVolume NiftiVolume::loadNifti(const QString &path) {
    nifti_image *nim = nifti_image_read(path.toStdString().c_str(), 1);
    if (!nim)
        throw std::runtime_error("Failed to read NIFTI file: " + path.toStdString());

    NiftiVolume vol;
    vol.file_path = path;

    const int nx = nim->nx;
    const int ny = nim->ny;
    const int nz = nim->nz;
    const int nt = (nim->nt > 1) ? nim->nt : 1;

    vol.spacing = Eigen::Vector3f(nim->dx > 0 ? nim->dx : 1.0f, nim->dy > 0 ? nim->dy : 1.0f,
                                  nim->dz > 0 ? nim->dz : 1.0f);

    const size_t totalVoxels = static_cast<size_t>(nx) * ny * nz * nt;
    std::vector<float> floatBuffer(totalVoxels);
    const void *srcData = nim->data;

    // Conversion des types de données vers float
    if (nim->datatype == NIFTI_TYPE_FLOAT32) {
        std::memcpy(floatBuffer.data(), srcData, totalVoxels * sizeof(float));
    } else {
        for (size_t i = 0; i < totalVoxels; ++i) {
            if (nim->datatype == NIFTI_TYPE_FLOAT64)
                floatBuffer[i] = static_cast<float>(static_cast<const double *>(srcData)[i]);
            else if (nim->datatype == NIFTI_TYPE_INT16)
                floatBuffer[i] = static_cast<float>(static_cast<const int16_t *>(srcData)[i]);
            else if (nim->datatype == NIFTI_TYPE_UINT16)
                floatBuffer[i] = static_cast<float>(static_cast<const uint16_t *>(srcData)[i]);
            else if (nim->datatype == NIFTI_TYPE_UINT8)
                floatBuffer[i] = static_cast<float>(static_cast<const uint8_t *>(srcData)[i]);
        }
    }

    // --- LOGIQUE COHÉRENTE : Tout en ColMajor ---
    // NIfTI stocke [X][Y][Z][T].
    // En ColMajor(nx, ny, nz, nt), la mémoire est exactement ordonnée comme [X][Y][Z][T].
    // Plus besoin de shuffle complexe. On définit juste les dimensions.

    vol.data = Eigen::Tensor<float, 4, Eigen::ColMajor>(nx, ny, nz, nt);
    std::memcpy(vol.data.data(), floatBuffer.data(), totalVoxels * sizeof(float));

    nifti_image_free(nim);
    return vol;
}

bool NiftiVolume::saveNifti(const QString &path, const NiftiVolume &vol) {
    nifti_image *nim = nifti_simple_init_nim();
    if (!nim)
        throw std::runtime_error("Failed to init nifti_image");

    const int nx = vol.data.dimension(0);
    const int ny = vol.data.dimension(1);
    const int nz = vol.data.dimension(2);
    const int nt = vol.data.dimension(3);

    nim->dim[0] = 4;
    nim->dim[1] = nx;
    nim->dim[2] = ny;
    nim->dim[3] = nz;
    nim->dim[4] = nt;
    nim->dim[5] = 1;
    nim->dim[6] = 1;
    nim->dim[7] = 1;
    nim->nx = nx;
    nim->ny = ny;
    nim->nz = nz;
    nim->nt = nt;
    nim->nvox = static_cast<size_t>(nx) * ny * nz * nt;

    nim->pixdim[1] = vol.spacing.x();
    nim->pixdim[2] = vol.spacing.y();
    nim->pixdim[3] = vol.spacing.z();
    nim->dx = nim->pixdim[1];
    nim->dy = nim->pixdim[2];
    nim->dz = nim->pixdim[3];

    nim->datatype = NIFTI_TYPE_FLOAT32;
    nim->nbyper = sizeof(float);
    nim->data = std::malloc(nim->nvox * nim->nbyper);

    // Comme le tenseur est déjà en (nx, ny, nz, nt),
    // le buffer mémoire est déjà parfaitement aligné pour NIfTI.
    std::memcpy(nim->data, vol.data.data(), nim->nvox * sizeof(float));

    nifti_set_filenames(nim, path.toStdString().c_str(), 0, 1);
    nifti_image_write(nim);
    nifti_image_free(nim);

    // --- RELECTURE DEBUG ---
    nifti_image *check = nifti_image_read(path.toStdString().c_str(), 0);
    if (check) {
        qDebug() << "[SAVE CHECK]" << path << "Dims:" << check->nx << check->ny << check->nz
                 << check->nt;
        nifti_image_free(check);
    }

    return true;
}

std::vector<float> NiftiVolume::toVector() const {
    return std::vector<float>(data.data(), data.data() + data.size());
}

std::vector<int64_t> NiftiVolume::getShape() const {
    return {
        static_cast<int64_t>(data.dimension(0)), // X
        static_cast<int64_t>(data.dimension(1)), // Y
        static_cast<int64_t>(data.dimension(2)), // Z
        static_cast<int64_t>(data.dimension(3))  // C
    };
}