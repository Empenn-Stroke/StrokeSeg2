#pragma once

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include <QString>

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
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    using Tensor4f = Eigen::Tensor<float, 4, Eigen::RowMajor>;

    /**
     * @brief Voxel data stored as a 4D Eigen tensor.
     *
     * Tensor layout is (C, X, Y, Z) in row-major order.
     * All voxel values are stored as floating-point values.
     */
    Tensor4f data; // (C, X, Y, Z)

    /**
     * @brief Physical voxel spacing in millimeters.
     *
     * spacing = (sx, sy, sz)
     */
    Eigen::Vector3f spacing; // (sx, sy, sz)

    QString file_path; // Original file path (optional)

    /**
     * @brief Create a NiftiVolume from voxel data and affine matrix. No file will
     * be created on disk; you might want to use @ref saveNifti for this.
     */
    static NiftiVolume createNifti(Tensor4f data, Eigen::Vector3f spacing = {});

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

    /**
     * @brief Vectorizes the volume tensor into a contiguous 1D array.
     *
     * The returned vector contains all tensor elements flattened in memory order
     * (row-major by default in Eigen::Tensor).
     *
     * This operation performs a copy of the underlying data.
     *
     * @return std::vector<float> Flattened tensor data
     *
     * @note The output order is consistent with Eigen::Tensor storage layout.
     *       Ensure compatibility with downstream libraries (e.g. ONNX, NumPy).
     */
    std::vector<float> toVector() const;

    /**
     * @brief Returns the shape of the volume tensor.
     *
     * The shape corresponds to the tensor dimensions in the following order:
     * - shape[0] = number of channels (C)
     * - shape[1] = size along X
     * - shape[2] = size along Y
     * - shape[3] = size along Z
     *
     * @return std::array<int, 4> Tensor dimensions (C, X, Y, Z)
     */
    std::vector<int64_t> getShape() const;
};