#include "preprocessor.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <nifti1_io.h>

namespace preprocessing {

    void Preprocessor::zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg) {
        double sum = 0.0, sq_sum = 0.0;
        size_t count = 0;

        std::vector<int64_t> shape = vol.getShape();

        const int C = vol.data.dimension(0);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);

        for (int c = 0; c < C; ++c)
            for (int x = 0; x < X; ++x)
                for (int y = 0; y < Y; ++y)
                    for (int z = 0; z < Z; ++z) {
                        if (seg && seg->data(c, x, y, z) < 0)
                            continue;
                        double v = vol.data(c, x, y, z);
                        sum += v;
                        sq_sum += v * v;
                        count++;
                    }

        if (count == 0)
            return;
        double mean = sum / count;
        double std = std::sqrt(std::max(sq_sum / count - mean * mean, 1e-8));

        for (int c = 0; c < C; ++c)
            for (int x = 0; x < X; ++x)
                for (int y = 0; y < Y; ++y)
                    for (int z = 0; z < Z; ++z) {
                        if (seg && seg->data(c, x, y, z) < 0)
                            continue;
                        vol.data(c, x, y, z) =
                            static_cast<float>((vol.data(c, x, y, z) - mean) / std);
                    }
    }

    std::vector<bool> Preprocessor::computeNonZeroMask(const NiftiVolume &vol) {
        const int C = vol.data.dimension(0);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);
        std::vector<bool> mask(X * Y * Z, false);

        for (int x = 0; x < X; ++x)
            for (int y = 0; y < Y; ++y)
                for (int z = 0; z < Z; ++z) {
                    bool nonzero = false;
                    for (int c = 0; c < C; ++c) {
                        if (vol.data(c, x, y, z) != 0.0f) {
                            nonzero = true;
                            break;
                        }
                    }
                    mask[x * Y * Z + y * Z + z] = nonzero;
                }
        return mask;
    }

    NiftiVolume Preprocessor::cropToNonZero(const NiftiVolume &vol, 
                                            const NiftiVolume *seg,
                                            int nonzero_label,
                                            std::array<std::array<int, 2>, 3> *bbox_out) {
        auto mask = computeNonZeroMask(vol);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);

        std::array<std::array<int, 2>, 3> bbox;
        if (bbox_out)
            bbox = *bbox_out;
        else {
            int x_min = X - 1, x_max = 0;
            int y_min = Y - 1, y_max = 0;
            int z_min = Z - 1, z_max = 0;
            bool found = false;
            for (int x = 0; x < X; ++x)
                for (int y = 0; y < Y; ++y)
                    for (int z = 0; z < Z; ++z)
                        if (mask[x * Y * Z + y * Z + z]) {
                            x_min = std::min(x_min, x);
                            x_max = std::max(x_max, x);
                            y_min = std::min(y_min, y);
                            y_max = std::max(y_max, y);
                            z_min = std::min(z_min, z);
                            z_max = std::max(z_max, z);
                            found = true;
                        }
            if (!found)
                throw std::runtime_error("All-zero volume, cannot crop");
            bbox = {{{x_min, x_max}, {y_min, y_max}, {z_min, z_max}}};
        }
        if (bbox_out)
            *bbox_out = bbox;

        NiftiVolume cropped;
        cropped.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(
            vol.data.dimension(0), bbox[0][1] - bbox[0][0] + 1, bbox[1][1] - bbox[1][0] + 1,
            bbox[2][1] - bbox[2][0] + 1);
        cropped.spacing = vol.spacing;

        for (int c = 0; c < vol.data.dimension(0); ++c)
            for (int x = 0; x < cropped.data.dimension(1); ++x)
                for (int y = 0; y < cropped.data.dimension(2); ++y)
                    for (int z = 0; z < cropped.data.dimension(3); ++z)
                        cropped.data(c, x, y, z) =
                            vol.data(c, x + bbox[0][0], y + bbox[1][0], z + bbox[2][0]);

        if (seg) {
            for (int c = 0; c < seg->data.dimension(0); ++c)
                for (int x = 0; x < cropped.data.dimension(1); ++x)
                    for (int y = 0; y < cropped.data.dimension(2); ++y)
                        for (int z = 0; z < cropped.data.dimension(3); ++z) {
                            float val =
                                seg->data(c, x + bbox[0][0], y + bbox[1][0], z + bbox[2][0]);
                            if (val == 0 && !mask[(x + bbox[0][0]) * Y * Z + (y + bbox[1][0]) * Z +
                                                  (z + bbox[2][0])])
                                val = nonzero_label;
                            cropped.data(c, x, y, z) = val;
                        }
        }

        return cropped;
    }

    std::pair<NiftiVolume, std::vector<std::array<int, 2>>>
    Preprocessor::padVolume(const NiftiVolume &vol, int min_size) {
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);
        const int C = vol.data.dimension(0);

        std::vector<std::array<int, 2>> padding(3);
        int dims[3] = {X, Y, Z};
        for (int i = 0; i < 3; ++i) {
            int total_pad = std::max(0, min_size - dims[i]);
            int pad_before = total_pad / 2;
            int pad_after = total_pad - pad_before;
            padding[i] = {pad_before, pad_after};
        }

        NiftiVolume padded;
        padded.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(C, X + padding[0][0] + padding[0][1],
                                                               Y + padding[1][0] + padding[1][1],
                                                               Z + padding[2][0] + padding[2][1]);
        padded.data.setZero();
        padded.spacing = vol.spacing;

        for (int c = 0; c < C; ++c)
            for (int x = 0; x < X; ++x)
                for (int y = 0; y < Y; ++y)
                    for (int z = 0; z < Z; ++z)
                        padded.data(c, x + padding[0][0], y + padding[1][0], z + padding[2][0]) =
                            vol.data(c, x, y, z);

        return {padded, padding};
    }

    QString Preprocessor::biasCorrect(const QString &input_path, const QString &prefix) {
        QString output_path = prefix + "_N4.nii.gz";

        QStringList args;
        args << "animaN4BiasCorrection" << "-i" << input_path << "-o"
             << output_path;

        int ret = wrapper.run(args);
        if (ret != 0) {
            std::string err_msg = wrapper.lastStderr().toStdString();
            if (err_msg.empty())
                err_msg = "Unknown error in AnimaWrapper";
            throw std::runtime_error("Bias correction failed: " + err_msg);
        }

        return output_path;
    }

    std::pair<QString, QString> Preprocessor::registerToReference(const QString &input_path,
                                                                  const QString &ref_path,
                                                                  const QString &prefix,
                                                                  const QString &suffix) {

        const QString output_path = prefix + "_" + suffix + ".nii.gz";
        const QString trsf_path = output_path.left(output_path.size() - 7) + ".txt";

        QStringList args;
        args << "animaPyramidalBMRegistration" 
            << "-i" << input_path 
            << "-m" << ref_path
            << "-o" << output_path 
            << "-O" << trsf_path;

        int ret = wrapper.run(args);
        if (ret != 0) {
            std::string err_msg = wrapper.lastStderr().toStdString();
            if (err_msg.empty())
                err_msg = "Unknown error in AnimaWrapper";
            throw std::runtime_error("Bias correction failed: " + err_msg);
        }

        return std::pair<QString, QString>(output_path, trsf_path);
    }

    PreprocessedVolume Preprocessor::preprocessModality(Preprocessor &pp,
                                                        const QString &modality_path,
                                                        bool is_MNI,
                                                        std::array<std::array<int, 2>, 3> *bbox_ptr) {
        PreprocessedVolume result;

        // --- Step 1: Define temporary prefix ---
        QString prefix = modality_path;
        prefix = prefix + "_preproc";

        QString img_path = modality_path;

        // --- Step 2: If not MNI, run bias correction, reorient, register ---
        QString MNI_output;
        QString trsf_path;
        if (!is_MNI) {
            // Bias correction
            MNI_output = pp.biasCorrect(modality_path, prefix);

            // Reorient to RAS (assuming you have a method in Preprocessor)
            MNI_output = pp.reorientToRAS(MNI_output, prefix);

            // Register to reference MNI
            std::tie(MNI_output, trsf_path) = pp.registerToReference(MNI_output, 
                                                                     pp.atlasImage, 
                                                                     "MNI", 
                                                                     prefix);
        } else {
            MNI_output = modality_path;
            trsf_path.clear();
        }

        result.trsf_path = trsf_path;

        // --- Step 3: Load NiftiVolume ---
        NiftiVolume vol = NiftiVolume::loadNifti(MNI_output);

        // --- Step 4: Crop to non-zero region ---
        NiftiVolume cropped;
        if (bbox_ptr) {
            cropped = pp.cropToNonZero(vol, nullptr, 1,
                                       bbox_ptr);
        } else {
            std::array<std::array<int, 2>, 3> computed_bbox;
            cropped = pp.cropToNonZero(vol,nullptr, 1, &computed_bbox);
        }

        result.original_shape =
            Eigen::Vector3i(vol.data.dimension(1), vol.data.dimension(2), vol.data.dimension(3));
        result.spacing = cropped.spacing;

        // --- Step 5: Resampling ---
        Eigen::Vector3f target_spacing(1.0f, 1.0f, 1.0f);
        result.data =
            pp.resampler.resample(cropped, target_spacing, false).data; // false = not segmentation

        // --- Step 6: Z-score normalization ---
        NiftiVolume norm_vol;
        norm_vol.data = result.data;
        norm_vol.spacing = result.spacing;
        pp.zScoreNormalize(norm_vol, nullptr);
        result.data = norm_vol.data;

        // --- Step 7: Padding to ensure min size ---
        NiftiVolume tmp_vol;
        tmp_vol.data = result.data;
        tmp_vol.spacing = result.spacing;
        auto pad_result = pp.padVolume(tmp_vol, 128);
        result.data = pad_result.first.data;
        result.padding = {pad_result.second[0], pad_result.second[1], pad_result.second[2]};

        // --- Step 8: Save MNI reference if needed ---
        QVariant keepMNI = pp.config.get("keep_MNI", true);
        if (keepMNI.toBool()) {
            result.MNI_base_image = MNI_output;
        }

        return result;


    }

    
    NiftiVolume Preprocessor::preprocess(const NiftiVolume &vol,
                                         const Eigen::Vector3f &target_spacing,
                                         bool is_segmentation) {
        // --- Step 1: Brain extraction ---
        QString tmp_prefix = "/tmp/preproc_" + QString::number(reinterpret_cast<uintptr_t>(&vol));
        QString brain_img_path = vol.file_path;
        if (!is_segmentation && brainExtraction) {
            brain_img_path =
                brainExtraction->run(QString::fromStdString(vol.file_path.toStdString()), tmp_prefix);
        }

        // --- Step 2: Load volume after brain extraction (or use input) ---
        NiftiVolume brain_vol = brain_img_path.isEmpty() ? vol : NiftiVolume::loadNifti(brain_img_path);

        // --- Step 3: Crop to non-zero region ---
        NiftiVolume cropped = cropToNonZero(brain_vol, nullptr);

        // --- Step 4: Z-score normalization ---
        if (!is_segmentation) {
            zScoreNormalize(cropped, nullptr);
        }

        // --- Step 5: Resampling ---
        NiftiVolume resampled;
        if (!is_segmentation) {
            resampled = resampler.resample(cropped, target_spacing, false); // image
        } else {
            resampled = resampler.resample(cropped, target_spacing, true); // segmentation
        }

        return resampled;
    }

    
    void Preprocessor::printAction(const QString &actionName) {
        spdlog::info("Starting {}...²", actionName.toStdString());
    }


    
} // namespace preprocessing