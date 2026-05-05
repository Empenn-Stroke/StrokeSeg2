#include "postprocessing/postprocessor.h"

#include <QString>
#include <optional>
#include <spdlog/spdlog.h>

#include "utils/log.h"
#include "managers/progressManager.h"

namespace 
{
    using namespace postprocessing;

    struct Segmentation 
    {
        Eigen::Tensor<float, 3, Eigen::ColMajor> seg;
        std::optional<Eigen::Tensor<float, 3, Eigen::ColMajor>> pmap;
    };

    /// @brief Save the nifti image. The name is dynamically constructed.
    /// @param dir Directory where the image will be saved
    /// @param data Image data
    /// @param base_name Base name of the image without extension
    /// @param suffix filename suffix. Note however that this is not the extension
    /// @return Path to the saved file
    QString save_img(QString dir, const Eigen::Vector3f &spacing,
                     const NiftiVolume::Tensor4f &data,
                     QString base_name,
                     QString suffix) 
    {
        QString output_path = dir + "/" + base_name + "_" + suffix + ".nii.gz";
        NiftiVolume output;
        output.data = data;
        output.spacing = spacing;
        NiftiVolume::saveNifti(output_path, output);
        return output_path;
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
                                         bool save_pmap) 
    {
        // extract lesion
        Eigen::Tensor<float, 3, Eigen::ColMajor> lesion = data.chip<3>(0);

        // softmax
        // we first substract the maximum value to each element to prevent exponential overflow
        //auto lesion_shifted = lesion - lesion.maximum();
        //auto lesion_exp_sum = lesion_shifted.sum();
        //Eigen::Tensor<float, 3, Eigen::ColMajor> pmap = lesion.unaryExpr([&](float val) {
        //    return exp(val) / lesion_exp_sum; //
        //});

        // Sigmoid (since we only have one channel, softmax is equivalent to sigmoid)
        Eigen::Tensor<float, 3, Eigen::ColMajor> pmap =
            lesion.unaryExpr([](float val) { return 1.0f / (1.0f + std::exp(-val)); });

        // calculate segmentation of lesion (channel 1)
        Eigen::Tensor<float, 3, Eigen::ColMajor> seg = (lesion >= threshold).cast<float>();

        return {
            .seg = seg,
            .pmap = (save_pmap) ? std::make_optional(pmap) : std::nullopt,
        };
    }

    /// @brief Remove specified padding from a segmentation. The padding is represented by an array
    /// of pairs representing the intervals to keep for each dimension.
    /// @param segmentation Segmentation data. This will be modified.
    /// @param padding Padding to use
    Segmentation remove_padding(const Segmentation &segmentation,
                                const std::array<std::array<int, 2>, 3> &padding) 
    {
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
                        ? std::make_optional<Eigen::Tensor<float, 3, Eigen::ColMajor>>(
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
                                  const Eigen::Vector3i &original_shape) 
    {
        // create full volume
        Segmentation full_volume{
            .seg = Eigen::Tensor<float, 3, Eigen::ColMajor>(original_shape[0], original_shape[1],
                                                            original_shape[2]),
            .pmap = (segmentation.pmap.has_value())
                        ? std::make_optional<Eigen::Tensor<float, 3, Eigen::ColMajor>>(
                              Eigen::Tensor<float, 3, Eigen::ColMajor>(
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
            full_volume.pmap->slice(offsets, extents) = *(segmentation.pmap);
        }

        return full_volume;
    }

    /// @brief Convert a segmentation result into a NiftiVolume instance. The segmentation will be
    /// expanded into a higher dimension, resulting in a single-channel 3D volume.
    /// @param segmentation Segmentation struct
    /// @param spacing
    /// @return NiftiVolume instance
    NiftiVolume segmentation_to_nifti_volume(const Segmentation &segmentation,
                                             Eigen::Vector3f spacing) 
    {
        auto &seg = segmentation.seg;
        Eigen::array<Eigen::Index, 4> new_shape{seg.dimension(0), seg.dimension(1),
                                                seg.dimension(2), 1};

        NiftiVolume::Tensor4f data = seg.reshape(new_shape);
        return {
            .data = data,
            .spacing = spacing,
        };
    }

    /// @brief Get data from NiftiVolume as a Tensor3f
    /// @param volume NiftiVolume instance
    /// @return data as Tensor3f
    Eigen::Tensor<float, 3, Eigen::ColMajor> nifti_volume_to_tensor3f(const NiftiVolume &volume) 
    {
        return volume.data.chip<3>(0);
    }

    void applyInverseRegistration(AnimaWrapper *wrapper, const QString &input_path,
                                  const QString &trsf_txt_path, const QString &patient_ref_path,
                                  const QString &output_path, const QString &interp = "nearest") {

        QString xml_path = trsf_txt_path;
        if (xml_path.endsWith(".txt")) {
        xml_path.replace(".txt", ".xml");
        } else {
            xml_path += ".xml";
        }

        if (!QFile::exists(xml_path)) {
            QStringList xmlArgs = {"animaTransformSerieXmlGenerator", "-i", trsf_txt_path, "-o",
                                   xml_path};
            if (wrapper->run(xmlArgs) != 0) {
                throw std::runtime_error("XML Generation failed: " +
                                         wrapper->lastStderr().toStdString());
            }
        }

        // --- Étape B : animaApplyTransformSerie ---
        // Applique la transformation inverse (-I)
        QString output_patient_path = input_mni_path;
        output_patient_path.replace(".nii.gz", "_to_patient.nii.gz");

        QStringList applyArgs;
        applyArgs << "animaApplyTransformSerie"
                  << "-i" << input_path << "-t" << xml_path << "-o" << output_path << "-g"
                  << patient_ref_path << "-I"
                  << "-n" << interp;

        printAction(
            QString("Applying inverse registration to %1").arg(QFileInfo(input_path).fileName()));
        if (wrapper->run(applyArgs) != 0) {
            throw std::runtime_error("Inverse registration failed for " + input_path.toStdString() +
                                     ": " + wrapper->lastStderr().toStdString());
        }

        return output_patient_path;
    }

}; // namespace


/**
 * @brief Apply postprocessing pipeline on the data produced by the inference step:
 *
 * - Convert the pmap to segmentation data. The pmap can also be returned as is
 * - Remove padding
 * - Uncrop
 * - Resample to the original spacing
 * - Save image
 * - Register to reference only if the inverse transformation was applied during
 *   preprocessing
 */
NiftiVolume postprocessing::Postprocessor::postprocess(const NiftiVolume::Tensor4f &data,
                                                        const PreprocessedVolume &preproc_volume,
                                                        const std::array<std::array<int, 2>, 3> &bbox,
                                                        float segmentation_threshold, bool save_pmap,
                                                        QString dir, QString trsf_path, bool mni) 
{

    Eigen::Vector3f debug_spacing = preproc_volume.spacing;

    // --- Step 1: Convert to segmentation ---

    ProgressManager::instance().report(93, 7, 0, new QString("Converting to segmentation"));

    printAction("Convert to segmentation");
    Segmentation segmentation = convert_to_segmentation(data, segmentation_threshold, save_pmap);
    
    // DEBUG SAVE 1
    if (m_save_intermediary_steps) {
        save_img(dir, debug_spacing, segmentation_to_nifti_volume(segmentation, debug_spacing).data,
                 QFileInfo(preproc_volume.original_t1_path).baseName(), "after_convert");
    }

    ProgressManager::instance().report(93, 7, 10, new QString("Removing padding"));

    // --- Step 2: Remove padding ---
    printAction("Remove padding");
    segmentation = remove_padding(segmentation, preproc_volume.padding);
    
    // DEBUG SAVE 2
    if (m_save_intermediary_steps) {
        save_img(dir, debug_spacing, segmentation_to_nifti_volume(segmentation, debug_spacing).data,
                 QFileInfo(preproc_volume.original_t1_path).baseName(), "after_unpad");
    }

    ProgressManager::instance().report(93, 7, 20, new QString("Returning to initial dimensions"));

    // --- Step 3: Uncrop ---
    printAction("Uncrop");
    segmentation = uncrop_from_bbox(segmentation, bbox, preproc_volume.original_shape);

    NiftiVolume segmentation_as_nifti = segmentation_to_nifti_volume(segmentation, debug_spacing);
    
    // DEBUG SAVE 3
    if (m_save_intermediary_steps) {
        save_img(dir, debug_spacing, segmentation_as_nifti.data,
                 QFileInfo(preproc_volume.original_t1_path).baseName(), "after_uncrop");
    }

    // --- Step 4: Resample ---
    ProgressManager::instance().report(93, 7, 30, new QString("Resampling to initial spacing"));

    printAction("Resample");
    Eigen::Vector3f target_spacing{1, 1, 1};

    Eigen::Tensor<float, 4, Eigen::ColMajor> temp_shuffled =
        segmentation_as_nifti.data.shuffle(Eigen::array<int, 4>{3, 0, 1, 2});

    // Resample takes C;X;Y;Z but we have X;Y;Z;C, so we need to shuffle the axes
    // before resampling, and then shuffle back after resampling
    // TODO : update resampler to work in X;Y;Z;C order to avoid these shuffles

    segmentation_as_nifti.data = temp_shuffled;

    try {
        segmentation_as_nifti = m_resampler.resample(segmentation_as_nifti, preproc_volume.spacing);
    } catch (const std::bad_alloc &e) {
        spdlog::error("Resampling failed: Out of memory. Check dimensions!");
        throw;
    }

    Eigen::Tensor<float, 4, Eigen::ColMajor> temp_back =
        segmentation_as_nifti.data.shuffle(Eigen::array<int, 4>{1, 2, 3, 0});
    
    segmentation_as_nifti.data = temp_back;

    segmentation.seg = nifti_volume_to_tensor3f(segmentation_as_nifti);

    ProgressManager::instance().report(93, 7, 70);

    // DEBUG SAVE 4
    if (m_save_intermediary_steps) {
        save_img(dir, debug_spacing, segmentation_as_nifti.data,
                 QFileInfo(preproc_volume.original_t1_path).baseName(), "after_resampling");
    }

    QString tmp_mni_path = dir + "/" + QFileInfo(preproc_volume.original_t1_path).baseName() + "_reconstructed_mni.nii.gz";

    // Copy save the data of the segmentation in MNI space, using the reference T1 header to ensure
    // correct orientation and spacing metadata.
    NiftiVolume::saveNiftiWithReference(tmp_mni_path, segmentation_as_nifti, Paths::atlasDir().filePath("Reference_T1.nii.gz"));

    if (mni) {
        return segmentation_as_nifti;
    }

    ProgressManager::instance().report(93, 7, 80);

    // --- Étape 5 : Registration inverse ---
    try {

        QString base_name = QFileInfo(preproc_volume.original_t1_path).baseName();
        QString final_patient_seg_path = dir + "/" + base_name + "_segmentation_to_patient.nii.gz";

        applyInverseRegistration(m_wrapper, tmp_mni_path, trsf_path, preproc_volume.original_t1_path,
                                 final_patient_seg_path, "nearest");

        if (save_pmap && segmentation.pmap.has_value()) {
            ProgressManager::instance().report(93, 7, 90,
                                               new QString("Processing probability map"));

            Segmentation pmap_struct = {.seg = *segmentation.pmap, .pmap = std::nullopt};
            NiftiVolume pmap_nii =
                segmentation_to_nifti_volume(pmap_struct, preproc_volume.spacing);

            QString tmp_pmap_mni = dir + "/" + base_name + "_pmap_mni_temp.nii.gz";
            NiftiVolume::saveNiftiWithReference(tmp_pmap_mni, pmap_nii,
                                                Paths::atlasDir().filePath("Reference_T1.nii.gz"));

            QString final_pmap_path = dir + "/" + base_name + "_pmap.nii.gz";
            applyInverseRegistration(m_wrapper, tmp_pmap_mni, trsf_path,
                                     preproc_volume.original_t1_path,
                                     final_pmap_path, "linear");

            QFile::remove(tmp_pmap_mni);
        }
        return NiftiVolume::loadNifti(final_patient_seg_path);

        ProgressManager::instance().report(93, 7, 100);
    } catch (const std::exception &e) { 
        qCritical("Critical error during inverse registration: %s", e.what());
        return;
    }
}
