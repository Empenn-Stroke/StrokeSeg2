#pragma once

#include <Eigen/Core>
#include <QString>
#include <unsupported/Eigen/CXX11/Tensor>
#include <vector>

/**
 * @struct NiftiVolume
 * @brief Container for a 4D NIFTI medical volume using Eigen tensors.
 *
 * This structure encapsulates voxel data and spatial metadata of a NIFTI
 * volume. The voxel intensities are stored in a 4D Eigen tensor with the
 * layout (X, Y, Z, C), where C corresponds to channels.
 *
 * The spacing vector defines the physical voxel size in millimeters
 * along each spatial axis (x, y, z).
 */
struct NiftiVolume {
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using Tensor4f = Eigen::Tensor<float, 4, Eigen::ColMajor>;
    /**
     * @brief Voxel data stored as a 4D Eigen tensor.
     *
     * Tensor layout is (X, Y, Z, C) in column-major order.
     * This layout matches the native NIfTI disk storage, where X is the
     * fastest-varying dimension in memory.
     */
    Tensor4f data; // (X, Y, Z, C)

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
     * Eigen tensor. Supported input datatypes include INT16, UINT8, FLOAT32, etc.
     *
     * @param path Path to the input NIFTI file.
     * @return NiftiVolume Loaded volume with voxel data and spacing initialized.
     *
     * @throws std::runtime_error If the file cannot be read or if the datatype
     * is not supported.
     */
    static NiftiVolume loadNifti(const QString &path);

    /**
     * @brief Save a NIFTI file to disk.
     *
     * Writes the provided NiftiVolume to a NIFTI-1 file. The voxel spacing
     * metadata is preserved. Data are saved as FLOAT32.
     *
     * @param path Output file path.
     * @param vol Volume to save.
     * * @return bool True if the file was saved successfully.
     *
     * @throws std::runtime_error If the file cannot be written.
     */
    static bool saveNifti(const QString &path, const NiftiVolume &vol);

    /**
     * @brief Vectorizes the volume tensor into a contiguous 1D array.
     *
     * The returned vector contains all tensor elements flattened in memory order
     * (column-major/Fortran-style by default in this configuration).
     *
     * This operation performs a copy of the underlying data.
     *
     * @return std::vector<float> Flattened tensor data
     *
     * @note The output order is consistent with NIfTI's [X][Y][Z][C] storage.
     * Ensure compatibility with downstream libraries (e.g. ONNX, NumPy).
     */
    std::vector<float> toVector() const;

    /**
     * @brief Returns the shape of the volume tensor.
     *
     * The shape corresponds to the tensor dimensions in the following order:
     * - shape[0] = size along X
     * - shape[1] = size along Y
     * - shape[2] = size along Z
     * - shape[3] = number of channels (C)
     *
     * @return std::vector<int64_t> Tensor dimensions (X, Y, Z, C)
     */
    std::vector<int64_t> getShape() const;
};