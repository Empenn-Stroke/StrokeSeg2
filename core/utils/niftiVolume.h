#pragma once

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

class QString;


/**
 * @struct NiftiVolume
 * @brief Container for a 4D NIFTI medical volume using Eigen tensors.
 *
 * This structure encapsulates voxel data and spatial metadata of a NIFTI
 * volume. The voxel intensities are stored in a 4D Eigen tensor with the
 * layout (C, X, Y, Z), where C corresponds to channels or time frames.
 *
 * The spacing vector defines the physical voxel size in millimeters
 * along each spatial axis.
 */
struct NiftiVolume {
    using Tensor4f = Eigen::Tensor<float, 4, Eigen::RowMajor>;


    /**
     * @brief Voxel data stored as a 4D Eigen tensor.
     *
     * Tensor layout is (C, X, Y, Z) in row-major order.
     * All voxel values are stored as floating-point values.
     */
    Tensor4f data;           // (C, X, Y, Z)

    /**
     * @brief Physical voxel spacing in millimeters.
     *
     * spacing = (sx, sy, sz)
     */
    Eigen::Vector3f spacing; // (sx, sy, sz)

    QString file_path; // Original file path (optional)

    /**
     * @brief Load a NIFTI file from disk.
     *
     * Reads a NIFTI-1 file and converts its voxel data into a floating-point
     * Eigen tensor. Supported input datatypes include INT16, UINT8 and FLOAT32.
     *
     * @param path(QString) Path to the input NIFTI file.
     * @return NiftiVolume Loaded volume with voxel data and spacing initialized.
     *
     * @throws std::runtime_error If the file cannot be read or if the datatype
     *         is not supported.
     */
    static NiftiVolume loadNifti(const QString &path);

    /**
     * @brief Save a NIFTI file to disk.
     *
     * Writes the provided NiftiVolume to a NIFTI-1 file. The voxel spacing
     * metadata is preserved. Data are saved as FLOAT32.
     *
     * @param path(QString) Output file path.
     * @param vol(NiftiVolume) Volume to save.
     *
     * @throws std::runtime_error If the file cannot be written.
     */
    static void saveNifti(const QString &path, const NiftiVolume &vol);
};