// SPDX-License-Identifier: AGPL-3.0-or-later

#include "postprocessing/postprocessor.h"

#include <QString>
#include <optional>

#include "managers/progressManager.h"
#include "utils/log.h"

namespace {
    using namespace postprocessing;

    struct Segmentation {
        Eigen::Tensor<float, 3, Eigen::ColMajor> seg;
        std::optional<Eigen::Tensor<float, 3, Eigen::ColMajor>> pmap;
    };

    QString saveImg(QString dir, const Eigen::Vector3f &spacing, const NiftiVolume::Tensor4f &data, QString base_name, QString suffix) 
    {
        QString output_path = dir + "/" + base_name + "_" + suffix + ".nii.gz";
        NiftiVolume output;
        output.data = data;
        output.spacing = spacing;
        NiftiVolume::saveNifti(output_path, output);
        return output_path;
    }

    Segmentation convertToSegmentation(NiftiVolume::Tensor4f data, float threshold, bool save_pmap) 
    {
        Eigen::Tensor<float, 3, Eigen::ColMajor> bg_logits = data.chip<3>(0);
        Eigen::Tensor<float, 3, Eigen::ColMajor> lesion_logits = data.chip<3>(1);

        Eigen::Tensor<float, 3, Eigen::ColMajor> pmap = (lesion_logits - bg_logits).unaryExpr([](float val) 
        {
            return 1.0f / (1.0f + std::exp(-val));
        });

        Eigen::Tensor<float, 3, Eigen::ColMajor> seg = (pmap >= threshold).cast<float>();

        return 
        {
            .seg = seg,
            .pmap = (save_pmap) ? std::make_optional(pmap) : std::nullopt,
        };
    }

    Segmentation removePadding(const Segmentation &segmentation,
                                const std::array<std::array<int, 2>, 3> &padding) 
    {
        std::array<int, 3> offsets{};
        std::array<int, 3> extents{};
        for (size_t i = 0; i < padding.size(); i++) 
        {
            offsets[i] = padding[i][0];
            extents[i] = padding[i][1] - padding[i][0];
        }

        return
        {
            .seg = segmentation.seg.slice(offsets, extents),
            .pmap = (segmentation.pmap.has_value())
                        ? std::make_optional<Eigen::Tensor<float, 3, Eigen::ColMajor>>(
                              segmentation.pmap->slice(offsets, extents))
                        : std::nullopt,
        };
    }

    Segmentation uncropFromBbox(const Segmentation &segmentation, const std::array<std::array<int, 2>, 3> &bbox, const Eigen::Vector3i &originalShape) 
    {
        Segmentation full_volume
        {
            .seg = Eigen::Tensor<float, 3, Eigen::ColMajor>(originalShape[0], originalShape[1], originalShape[2]),
            .pmap = (segmentation.pmap.has_value())
                        ? std::make_optional<Eigen::Tensor<float, 3, Eigen::ColMajor>>(
                              Eigen::Tensor<float, 3, Eigen::ColMajor>(originalShape[0], originalShape[1], originalShape[2]))
                        : std::nullopt,
        };

        full_volume.seg.setZero();
        if (full_volume.pmap.has_value()) 
        {
            full_volume.pmap->setZero();
        }

        std::array<int, 3> offsets{};
        std::array<int, 3> extents{};

        for (size_t i = 0; i < bbox.size(); i++) 
        {
            offsets[i] = bbox[i][0];
            extents[i] = std::min((int)segmentation.seg.dimension(i), originalShape[i] - offsets[i]);
        }

        std::array<int, 3> src_offsets{0, 0, 0};

        full_volume.seg.slice(offsets, extents) = segmentation.seg.slice(src_offsets, extents);

        if (full_volume.pmap.has_value()) 
        {
            full_volume.pmap->slice(offsets, extents) = segmentation.pmap->slice(src_offsets, extents);
        }

        return full_volume;
    }

    NiftiVolume segmentationToNiftiVolume(const Segmentation &segmentation, Eigen::Vector3f spacing) 
    {
        auto &seg = segmentation.seg;
        Eigen::array<Eigen::Index, 4> new_shape{seg.dimension(0), seg.dimension(1), seg.dimension(2), 1};

        NiftiVolume::Tensor4f data = seg.reshape(new_shape);

        return 
        {
            .data = data,
            .spacing = spacing,
        };
    }

    Eigen::Tensor<float, 3, Eigen::ColMajor> niftiVolumeToTensor3f(const NiftiVolume &volume) 
    {
        return volume.data.chip<3>(0);
    }

    void applyInverseRegistration(AnimaWrapper *wrapper, const QString &input_path, const QString &trsf_txt_path, const QString &patient_ref_path, const QString &output_path, const QString &interp = "nearest")
    {
        QString xml_path = trsf_txt_path;
        xml_path.replace(".txt", ".xml");

        if (!QFile::exists(xml_path)) 
        {
            QStringList xmlArgs = {"animaTransformSerieXmlGenerator", "-i", trsf_txt_path, "-o", xml_path};
            if (wrapper->run(xmlArgs) != 0) 
            {
                throw std::runtime_error("XML Generation failed: " + wrapper->lastStderr().toStdString());
            }
        }

        QStringList applyArgs;
        applyArgs << "animaApplyTransformSerie"
                  << "-i" << input_path << "-t" << xml_path << "-o" << output_path << "-g"
                  << patient_ref_path << "-I"
                  << "-n" << interp;

        printAction(QString("Applying inverse registration to %1").arg(QFileInfo(input_path).fileName()));
        if (wrapper->run(applyArgs) != 0)
        {
            throw std::runtime_error("Inverse registration failed for " + input_path.toStdString() + ": " + wrapper->lastStderr().toStdString());
        }
    }

}; // namespace

NiftiVolume postprocessing::Postprocessor::postprocess(const NiftiVolume::Tensor4f &data, const PreprocessedVolume &preproc_volume,
                                                        const std::array<std::array<int, 2>, 3> &bbox, float segmentation_threshold, bool save_pmap,
                                                        QString dir, QString trsfPath, bool mni, bool saveInterSteps) 
{
    NiftiVolume result;

    Eigen::Vector3f debug_spacing = (preproc_volume.spacing.minCoeff() > 0.0f) 
                                     ? preproc_volume.spacing 
                                     : Eigen::Vector3f(1.0f, 1.0f, 1.0f);

    // --- Step 1: Convert to segmentation ---
    ProgressManager::instance().report(93, 7, 0, new QString("Converting to segmentation"));
    printAction("Convert to segmentation");
    Segmentation segmentation = convertToSegmentation(data, segmentation_threshold, save_pmap);

    if (saveInterSteps) 
    {
        saveImg(dir, debug_spacing, segmentationToNiftiVolume(segmentation, debug_spacing).data, QFileInfo(preproc_volume.originalPath).baseName(), "after_convert");
    }

    // --- Step 2: Remove padding ---
    ProgressManager::instance().report(93, 7, 20, new QString("Removing padding"));

    printAction("Remove padding");
    segmentation = removePadding(segmentation, preproc_volume.padding);

    if (saveInterSteps) {
        saveImg(dir, debug_spacing, segmentationToNiftiVolume(segmentation, debug_spacing).data, QFileInfo(preproc_volume.originalPath).baseName(), "after_unpad");
    }

    ProgressManager::instance().report(93, 7, 40, new QString("Returning to initial dimensions"));

    printAction("Uncrop");
    segmentation = uncropFromBbox(segmentation, bbox, preproc_volume.originalShape);

    NiftiVolume segmentation_as_nifti = segmentationToNiftiVolume(segmentation, debug_spacing);

    if (saveInterSteps) {
        saveImg(dir, debug_spacing, segmentation_as_nifti.data, QFileInfo(preproc_volume.originalPath).baseName(), "after_uncrop");
    }

    ProgressManager::instance().report(93, 7, 70, new QString("Resampling to initial spacing"));

    printAction("Resample");

    Eigen::Tensor<float, 4, Eigen::ColMajor> temp_shuffled = segmentation_as_nifti.data.shuffle(Eigen::array<int, 4>{3, 0, 1, 2});

    segmentation_as_nifti.data = temp_shuffled;

    try {
        segmentation_as_nifti = m_resampler.resample(segmentation_as_nifti, Eigen::Vector3f(1.0f, 1.0f, 1.0f));
    } catch (const std::bad_alloc &e) {
        qCritical("Resampling failed: Out of memory. Check dimensions!");
        throw;
    }

    Eigen::Tensor<float, 4, Eigen::ColMajor> temp_back = segmentation_as_nifti.data.shuffle(Eigen::array<int, 4>{1, 2, 3, 0});

    segmentation_as_nifti.data = temp_back;

    segmentation.seg = niftiVolumeToTensor3f(segmentation_as_nifti);

    if (saveInterSteps) {
        saveImg(dir, debug_spacing, segmentation_as_nifti.data, QFileInfo(preproc_volume.originalPath).baseName(), "after_resampling");
    }

    ProgressManager::instance().report(93, 7, 70);

    QString tmp_mni_path = dir + "/" + QFileInfo(preproc_volume.originalPath).baseName() + "_reconstructed_mni.nii.gz";

    NiftiVolume::saveNiftiWithReference(tmp_mni_path, segmentation_as_nifti,
                                        Paths::atlasDir().filePath("Reference_T1.nii.gz"));

    if (mni) 
    {
        result = segmentation_as_nifti;
    }
    else
    {
        ProgressManager::instance().report(93, 7, 80);

        // --- Step 4: Inverse Registration vers l'espace patient ---
        try 
        {
            QString base_name = QFileInfo(preproc_volume.originalPath).baseName();
            QString final_patient_seg_path = dir + "/" + base_name + "_segmentation_to_patient.nii.gz";

            applyInverseRegistration(m_wrapper, tmp_mni_path, trsfPath, preproc_volume.originalPath, final_patient_seg_path, "nearest");

            if (save_pmap && segmentation.pmap.has_value()) 
            {
                ProgressManager::instance().report(93, 7, 90, new QString("Processing probability map"));

                Segmentation pmap_struct = {.seg = *segmentation.pmap, .pmap = std::nullopt};
                NiftiVolume pmap_nii = segmentationToNiftiVolume(pmap_struct, debug_spacing);

                QString tmp_pmap_mni = dir + "/" + base_name + "_pmap_mni_temp.nii.gz";
                NiftiVolume::saveNiftiWithReference(tmp_pmap_mni, pmap_nii, Paths::atlasDir().filePath("Reference_T1.nii.gz"));

                QString final_pmap_path = dir + "/" + base_name + "_pmap.nii.gz";
                applyInverseRegistration(m_wrapper, tmp_pmap_mni, trsfPath, preproc_volume.originalPath, final_pmap_path, "linear");

                QFile::remove(tmp_pmap_mni);
            }
            result = NiftiVolume::loadNifti(final_patient_seg_path);

        } 
        catch (const std::exception &e)
        {
            qCritical("Critical error during inverse registration: %s", e.what());
            throw;
        }
    }

    return result;
}