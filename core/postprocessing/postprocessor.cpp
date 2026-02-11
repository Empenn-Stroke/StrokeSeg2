#include <QString>
#include <optional>
#include <spdlog/spdlog.h>

#include "postprocessing/postprocessor.h"
#include "utils/log.h"

namespace {
    using namespace postprocessing;

    struct Segmentation {
        Eigen::Tensor<float, 3, Eigen::RowMajor> seg;
        std::optional<Eigen::Tensor<float, 3, Eigen::RowMajor>> pmap;
    };

    /// @brief Save the nifti image. The name is dynamically constructed.
    /// @param dir Directory where the image will be saved
    /// @param data Image data
    /// @param base_name Base name of the image without extension
    /// @param suffix filename suffix. Note however that this is not the extension
    /// @return Path to the saved file
    QString save_img(QString dir, const NiftiVolume::Tensor4f &data, QString base_name,
                     QString suffix) {
        QString output_path = dir + "/" + base_name + "_" + suffix + ".nii.gz";
        NiftiVolume output(data);
        NiftiVolume::saveNifti(output_path, output);
    }

    /// @brief Convert the output of the model into a segmentation based on the given threshhold.
    /// Can also return a probability map if requested.
    /// - Extract lesion channel
    /// - Apply a softmax to the lesion channel
    /// - Calculate the binary mask with the threshold param
    /// - Return the segmentation; and the pmap if requested
    ///
    /// @param data Model output. Shape is expected to be (C, X, Y, Z)
    /// @param threshold Threshold to create the binary mask
    /// @param save_pmap If true, save probability map alongside the segmentation
    /// @return Segmentation struct
    Segmentation convert_to_segmentation(NiftiVolume::Tensor4f data, float threshold,
                                         bool save_pmap) {
        // extract lesion
        Eigen::Tensor<float, 3, Eigen::RowMajor> lesion = data[1];

        // softmax
        // we first substract the maximum value to each element to prevent exponential overflow
        auto lesion_shifted = lesion - lesion.maximum();
        auto lesion_exp_sum = lesion_shifted.sum();
        Eigen::Tensor<float, 3, Eigen::RowMajor> pmap = lesion.unaryExpr([&](float val) {
            return exp(val) / lesion_exp_sum; //
        });

        // calculate segmentation of lesion (channel 1)
        Eigen::Tensor<float, 3, Eigen::RowMajor> seg = (lesion >= threshold);
        return {
            .seg = seg,
            .pmap = (save_pmap) ? std::make_optional<Eigen::Tensor<float, 3, Eigen::RowMajor>>(pmap)
                                : std::nullopt,
        };
    }

    /// @brief Remove specified padding from a segmentation. The padding is represented by an array
    /// of pairs representing the intervals to keep for each dimension.
    /// @param segmentation Segmentation data. This will be modified.
    /// @param padding Padding to use
    Segmentation remove_padding(const Segmentation &segmentation,
                                const std::array<std::array<int, 2>, 3> &padding) {
        // calculate offsets and extents (offset and length of slices)
        std::array<int, 3> offsets{};
        std::array<int, 3> extents{};
        for (int i = 0; i < padding.size(); i++) {
            offsets[i] = padding[i][0];
            extents[i] = padding[i][1] - padding[i][0];
        }

        // apply slices
        return {
            .seg = segmentation.seg.slice(offsets, extents),
            .pmap = (segmentation.pmap.has_value())
                        ? std::make_optional<Eigen::Tensor<float, 3, Eigen::RowMajor>>(
                              segmentation.pmap->slice(offsets, extents))
                        : std::nullopt,
        };
    }

    /// @brief Place the cropped data back into a full-size volume. Then, transpose the volume axes
    /// from (X, Y, Z) to (Z, Y, X)
    /// @param segmentation Segmentation data. This will be modified (re-allocated so that the size
    /// of its volumes fits the full size)
    /// @param bbox Bounding box coordinates used for cropping
    /// @param original_shape Original shape of the image before preprocessing
    Segmentation uncrop_from_bbox(const Segmentation &segmentation,
                                  const std::array<std::array<int, 2>, 3> &bbox,
                                  const Eigen::Vector3i &original_shape) {
        // create full volume
        Segmentation full_volume{
            .seg = Eigen::Tensor<float, 3, Eigen::RowMajor>(original_shape[0], original_shape[1],
                                                            original_shape[2]),
            .pmap = (segmentation.pmap.has_value())
                        ? std::make_optional<Eigen::Tensor<float, 3, Eigen::RowMajor>>(
                              Eigen::Tensor<float, 3, Eigen::RowMajor>(
                                  original_shape[0], original_shape[1], original_shape[2]))
                        : std::nullopt,
        };
        full_volume.seg.setZero();
        if (full_volume.pmap.has_value()) {
            full_volume.pmap->setZero();
        }

        // place cropped data inside full volume
        std::array<int, 3> offsets{};
        std::array<int, 3> extents{};
        for (int i = 0; i < bbox.size(); i++) {
            offsets[i] = bbox[i][0];
            extents[i] = bbox[i][1] - bbox[i][0];
        }

        full_volume.seg.slice(offsets, extents) = segmentation.seg;
        if (full_volume.pmap.has_value()) {
            full_volume.pmap->slice(offsets, extents) = segmentation.pmap;
        }
    }

    /// @brief Convert a segmentation result into a NiftiVolume instance. The segmentation will be
    /// expanded into a higher dimension, resulting in a single-channel 3D volume.
    /// @param segmentation Segmentation struct
    /// @param spacing
    /// @return NiftiVolume instance
    NiftiVolume segmentation_to_nifti_volume(const Segmentation &segmentation,
                                             Eigen::Vector3f spacing) {
        auto &seg = segmentation.seg;
        Eigen::array<Eigen::Index, 5> new_shape{1, seg.dimension(0), seg.dimension(1),
                                                seg.dimension(2), seg.dimension(3)};
        NiftiVolume::Tensor4f data = seg.reshape(new_shape);
        return {
            .data = data,
            .spacing = spacing,
        };
    }

    /// @brief Get data from NiftiVolume as a Tensor3f
    /// @param volume NiftiVolume instance
    /// @return data as Tensor3f
    Eigen::Tensor<float, 3, Eigen::RowMajor> nifti_volume_to_tensor3f(const NiftiVolume &volume) {
        return volume.data[0];
    }

}; // namespace

void postprocessing::Postprocessor::postprocess(const NiftiVolume::Tensor4f &data,
                                                const PreprocessedVolume &preproc_volume,
                                                const std::array<std::array<int, 2>, 3> &bbox,
                                                float segmentation_threshold, bool save_pmap,
                                                QString dir, QString trsf_path) {
    // --- Step 1: Convert to segmentation ---
    printAction("Convert to segmentation");
    Segmentation segmentation = convert_to_segmentation(data, segmentation_threshold, save_pmap);

    // --- Step 2: Convert to segmentation ---
    printAction("Remove padding");
    segmentation = remove_padding(segmentation, preproc_volume.padding);

    // --- Step 3: Uncrop ---
    printAction("Uncrop");
    segmentation = uncrop_from_bbox(segmentation, bbox, preproc_volume.original_shape);

    // --- Step 4: Resample ---
    printAction("Resample");
    Eigen::Vector3f new_spacing{1, 1, 1};
    NiftiVolume segmentation_as_nifti = segmentation_to_nifti_volume(segmentation, new_spacing);
    resampler.resample(segmentation_as_nifti, new_spacing);
    segmentation.seg = nifti_volume_to_tensor3f(segmentation_as_nifti);

    // --- Step 5: Save image ---
    printAction("Saving image to nii");
    QString nii_file = save_img(dir, segmentation_as_nifti.data, "azerty", "pmap");
    // TODO:
}