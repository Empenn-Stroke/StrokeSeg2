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

        int ret = wrapper->run(args);
        if (ret != 0) {
            std::string err_msg = wrapper->lastStderr().toStdString();
            if (err_msg.empty())
                err_msg = "Unknown error in AnimaWrapper";
            throw std::runtime_error("Bias correction failed: " + err_msg);
        }

        return output_path;
    }

    std::pair<QString, QString> Preprocessor::registerToReference(const QString &input_path,
                                                                  const QString &ref_path,
                                                                  const QString &prefix_label,
                                                                  const QString &base_path_prefix) {

        QFileInfo fileInfo(base_path_prefix);
        QString output_path =
            fileInfo.absolutePath() + "/" + prefix_label + "_" + fileInfo.fileName() + ".nii.gz";
        const QString trsf_path = output_path.left(output_path.size() - 7) + ".txt";

        QStringList args;
        args << "animaPyramidalBMRegistration" 
            << "-i" << input_path 
            << "-m" << ref_path
            << "-o" << output_path 
            << "-O" << trsf_path;

        int ret = wrapper->run(args);
        if (ret != 0) {
            std::string err_msg = wrapper->lastStderr().toStdString();
            if (err_msg.empty())
                err_msg = "Unknown error in AnimaWrapper";
            throw std::runtime_error("Bias correction failed: " + err_msg);
        }

        return std::pair<QString, QString>(output_path, trsf_path);
    }

    PreprocessedVolume Preprocessor::preprocessModality(const QString &modality_path,
                                                        bool is_MNI,
                                                        std::array<std::array<int, 2>, 3> *bbox_ptr) {
        PreprocessedVolume result;

        // --- Step 1: Define temporary prefix ---
        QString prefix =
            QFileInfo(modality_path).absolutePath() + "/" + QFileInfo(modality_path).baseName();
        if (prefix.endsWith(".nii"))
            prefix.chop(4);
        prefix += "_preproc";

        QString img_path = modality_path;

        // --- Step 2: If not MNI, run bias correction, reorient, register ---
        QString MNI_output;
        QString trsf_path;
        if (!is_MNI) {
            // Bias correction
            printAction("bias correction");
            MNI_output = biasCorrect(modality_path, prefix);

            // Reorient to RAS (assuming you have a method in Preprocessor)
            //printAction("reorient to RAS");
            //MNI_output = reorientToRAS(MNI_output, prefix);

            // Register to reference MNI
            printAction("register to MNI");
            std::tie(MNI_output, trsf_path) = registerToReference(MNI_output, 
                                                                     atlasImage, 
                                                                     prefix, 
                                                                     "MNI");
        } else {
            MNI_output = modality_path;
            trsf_path.clear();
        }

        result.trsf_path = trsf_path;

        // --- Step 3: Load NiftiVolume ---
        printAction("loading NIFTI volume");
        NiftiVolume vol = NiftiVolume::loadNifti(MNI_output);

        // --- Step 4: Crop to non-zero region ---
        NiftiVolume cropped;
        if (bbox_ptr) {
            cropped = cropToNonZero(vol, nullptr, 1,
                                       bbox_ptr);
        } else {
            std::array<std::array<int, 2>, 3> computed_bbox;
            cropped = cropToNonZero(vol,nullptr, 1, &computed_bbox);
        }

        result.original_shape =
            Eigen::Vector3i(vol.data.dimension(1), vol.data.dimension(2), vol.data.dimension(3));
        result.spacing = cropped.spacing;

        // --- Step 5: Resampling ---
        printAction("resampling to 1mm isotropic");
        Eigen::Vector3f target_spacing(1.0f, 1.0f, 1.0f);
        result.data =
            resampler.resample(cropped, target_spacing, false).data; // false = not segmentation

        // --- Step 6: Z-score normalization ---
        printAction("z-score normalization");
        NiftiVolume norm_vol;
        norm_vol.data = result.data;
        norm_vol.spacing = result.spacing;
        zScoreNormalize(norm_vol, nullptr);
        result.data = norm_vol.data;

        // --- Step 7: Padding to ensure min size ---
        printAction("padding");
        NiftiVolume tmp_vol;
        tmp_vol.data = result.data;
        tmp_vol.spacing = result.spacing;
        auto pad_result = padVolume(tmp_vol, 128);
        result.data = pad_result.first.data;
        result.padding = {pad_result.second[0], pad_result.second[1], pad_result.second[2]};

        // --- Step 8: Save MNI reference if needed ---
        QVariant keepMNI = config.get("keep_MNI", true);
        if (keepMNI.toBool()) {
            printAction("saving MNI base image");
            result.MNI_base_image = MNI_output;
        }

        return result;


    }
    
    PreprocessedVolume Preprocessor::preprocess(const QString &t1_path, const QString &flair_path,
                                                const QString &temp_dir, bool bet_only) {
        preprocessing_steps.clear(); // équivalent de self.preprocessing_steps
        QString prefix = temp_dir + "/" + QFileInfo(t1_path).baseName();
        QString bet_t1_path;

        // --- Brain extraction T1 ---
        if (!prefix.endsWith(QString("BET")) && !prefix.endsWith(QString("MNI"))) {
            printAction("brain extraction");
            bet_t1_path = brainExtraction->run(t1_path, prefix);
        } else {
            // Si déjà BET ou MNI, juste copier
            bet_t1_path = temp_dir + "/" + QFileInfo(t1_path).fileName();
            QFile::copy(t1_path, bet_t1_path);
        }

        PreprocessedVolume t1_result;

        if (!bet_only) {
            bool is_MNI = prefix.endsWith(QString("MNI"));
            t1_result = preprocessModality(bet_t1_path, is_MNI);
            if (!is_MNI) {
                moveToOutput(bet_t1_path);
            }
        } else {
            moveToOutput(bet_t1_path);
        }

        // --- Option FLAIR ---
        PreprocessedVolume combined_result;
        if (!flair_path.isEmpty()) {
            QString flair_prefix = temp_dir + "/" + QFileInfo(flair_path).baseName();
            QString bet_flair_path;

            if (!flair_prefix.endsWith(QString("BET")) && !flair_prefix.endsWith(QString("MNI"))) {
                printAction("register FLAIR to T1");
                auto [flair_registered, _] =
                    registerToReference(flair_path, t1_path, flair_prefix, QString("T1"));
                preprocessing_steps.push_back(flair_registered);

                printAction("brain extraction");
                bet_flair_path = brainExtraction->run(flair_registered, flair_prefix);
            } else {
                bet_flair_path = temp_dir + "/" + QFileInfo(flair_path).fileName();
                QFile::copy(flair_path, bet_flair_path);
            }

            PreprocessedVolume flair_result;
            if (!bet_only) {
                bool is_MNI = flair_prefix.endsWith(QString("MNI"));
                flair_result =
                    preprocessModality(bet_flair_path, is_MNI, &t1_result.padding);
                if (!is_MNI)
                    moveToOutput(bet_flair_path);
            } else {
                moveToOutput(bet_flair_path);
            }

            // --- Combiner les deux canaux ---
            int C_t1 = t1_result.data.dimension(0);
            int C_flair = flair_result.data.dimension(0);
            int X = t1_result.data.dimension(1);
            int Y = t1_result.data.dimension(2);
            int Z = t1_result.data.dimension(3);

            Eigen::Tensor<float, 4, Eigen::RowMajor> combined(C_t1 + C_flair, X, Y, Z);
            combined.slice(Eigen::array<Eigen::Index, 4>{0, 0, 0, 0},
                           Eigen::array<Eigen::Index, 4>{C_t1, X, Y, Z}) = t1_result.data;
            combined.slice(Eigen::array<Eigen::Index, 4>{C_t1, 0, 0, 0},
                           Eigen::array<Eigen::Index, 4>{C_flair, X, Y, Z}) = flair_result.data;

            combined_result = t1_result;
            combined_result.data = combined;
            combined_result.padding = t1_result.padding;
            combined_result.MNI_base_image = t1_result.MNI_base_image;

            // --- Sauvegarder preprocessing steps ---
            if (config.get("save_preproc", true).toBool()) {
                for (auto &path : preprocessing_steps)
                    moveToOutput(path);
            }

            return combined_result;
        }

        // --- Si pas de FLAIR ---
        if (!bet_only && config.get("save_preproc", true).toBool()) {
            for (auto &path : preprocessing_steps)
                moveToOutput(path);
        }

        return t1_result;
    }

    
    void Preprocessor::printAction(const QString &actionName) {
        spdlog::info("Starting {}...", actionName.toStdString());
    }

    QString Preprocessor::moveToOutput(const QString &img_path) {
        ConfigManager &config = ConfigManager::instance();

        QString input_path = config.get("input_path", "").toString();
        bool is_file = config.get("is_file", true).toBool();

        QString filename = QFileInfo(img_path).fileName();
        QString subject_name = filename.split("_").first();

        QString output_dir;

        if (is_file) {
            if (input_path.contains(QString("rawdata"))) {
                // BIDS + file input
                QString raw_dir = input_path.section(QString("rawdata"), 0, 0);
                output_dir = raw_dir + QString("derivatives") + "/" + subject_name + "/anat";
            } else {
                // Non-BIDS file input
                output_dir = QFileInfo(input_path).absolutePath();
            }
        } else {
            // Directory input (BIDS)
            output_dir = input_path + "/" + QString("derivatives") + "/" + subject_name + "/anat";
        }

        QDir().mkpath(output_dir);

        QString dst = output_dir + "/" + filename;

        QFile::remove(dst); // overwrite if exists
        if (!QFile::copy(img_path, dst)) {
            throw std::runtime_error("Failed to copy file to output: " + dst.toStdString());
        }

        return dst;
    }



    
} // namespace preprocessing