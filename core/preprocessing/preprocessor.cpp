#include "preprocessor.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <nifti1_io.h>

namespace preprocessing {

    void Preprocessor::zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg) {
        const int C = vol.data.dimension(0);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);

        bool useSeg = (seg && seg->data.dimension(1) == X && seg->data.dimension(2) == Y &&
                       seg->data.dimension(3) == Z);

        double sum = 0.0;
        double sq_sum = 0.0;
        size_t count = 0;

        for (int c = 0; c < C; ++c) {
            for (int x = 0; x < X; ++x) {
                for (int y = 0; y < Y; ++y) {
                    for (int z = 0; z < Z; ++z) {
                        float v = vol.data(c, x, y, z);

                        if (useSeg) {
                            float segVal = seg->data(0, x, y, z);
                            // On ne skip que si on est CERTAIN d'être dans le fond (-1)
                            if (segVal < -0.5f)
                                continue;
                        } else {
                            if (std::abs(v) < 1e-5f)
                                continue;
                        }

                        sum += v;
                        sq_sum += (double)v * v;
                        count++;
                    }
                }
            }
        }

        if (count == 0) {
            spdlog::error("zScoreNormalize: Aucun voxel trouvé (Somme={}, Count={})", sum, count);
            return;
        }

        double mean = sum / count;
        double variance = (sq_sum / count) - (mean * mean);
        double std_dev = std::sqrt(std::max(variance, 1e-8));

        // Application de la normalisation
        float *dataPtr = vol.data.data();
        for (int i = 0; i < vol.data.size(); ++i) {
            dataPtr[i] = static_cast<float>((dataPtr[i] - mean) / std_dev);
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
                        if (std::abs(vol.data(c, x, y, z)) > 1e-5f) {
                            nonzero = true;
                            break;
                        }
                    }
                    mask[x * Y * Z + y * Z + z] = nonzero;
                }
        return mask;
    }

    std::pair<NiftiVolume, NiftiVolume>
    Preprocessor::cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg,
                                            int nonzero_label,
                                            std::array<std::array<int, 2>, 3> *bbox_out) {
        auto mask = computeNonZeroMask(vol);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);

        std::array<std::array<int, 2>, 3> bbox;

        if (bbox_out && (*bbox_out)[0][0] != -1) {
            bbox = *bbox_out;
        } else {
            int x_min = X, x_max = 0;
            int y_min = Y, y_max = 0;
            int z_min = Z, z_max = 0;
            bool found = false;

            for (int x = 0; x < X; ++x) {
                for (int y = 0; y < Y; ++y) {
                    for (int z = 0; z < Z; ++z) {
                        if (mask[x * Y * Z + y * Z + z]) {
                            if (x < x_min)
                                x_min = x;
                            if (x > x_max)
                                x_max = x;
                            if (y < y_min)
                                y_min = y;
                            if (y > y_max)
                                y_max = y;
                            if (z < z_min)
                                z_min = z;
                            if (z > z_max)
                                z_max = z;
                            found = true;
                        }
                    }
                }
            }

            if (!found)
                throw std::runtime_error("All-zero volume, cannot crop");

            bbox = {{{x_min, x_max + 1}, {y_min, y_max + 1}, {z_min, z_max + 1}}};
        }

        if (bbox_out)
            *bbox_out = bbox;

        int newX = bbox[0][1] - bbox[0][0];
        int newY = bbox[1][1] - bbox[1][0];
        int newZ = bbox[2][1] - bbox[2][0];

        NiftiVolume croppedData;
        croppedData.data =
            Eigen::Tensor<float, 4, Eigen::RowMajor>(vol.data.dimension(0), newX, newY, newZ);
        croppedData.spacing = vol.spacing;

        NiftiVolume croppedSeg;
        croppedSeg.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(1, newX, newY, newZ);
        croppedSeg.spacing = vol.spacing;

        // 1. Remplissage de l'Image (Data)
        for (int c = 0; c < vol.data.dimension(0); ++c) {
            for (int x = 0; x < newX; ++x) {
                int old_x = x + bbox[0][0];
                for (int y = 0; y < newY; ++y) {
                    int old_y = y + bbox[1][0];
                    for (int z = 0; z < newZ; ++z) {
                        int old_z = z + bbox[2][0];
                        croppedData.data(c, x, y, z) = vol.data(c, old_x, old_y, old_z);
                    }
                }
            }
        }

        // 2. Remplissage du Segmentation Mask (Seg) - COPIE STRICTE PYTHON
        for (int x = 0; x < newX; ++x) {
            int old_x = x + bbox[0][0];
            for (int y = 0; y < newY; ++y) {
                int old_y = y + bbox[1][0];
                for (int z = 0; z < newZ; ++z) {
                    int old_z = z + bbox[2][0];

                    bool is_nonzero = mask[old_x * Y * Z + old_y * Z + old_z];

                    if (seg) {
                        // Si on a un seg en entrée (ex: cas du FLAIR qui réutilise le seg du T1)
                        float val = seg->data(0, old_x, old_y, old_z);
                        if (val == 0 && !is_nonzero)
                            val = nonzero_label;
                        croppedSeg.data(0, x, y, z) = val;
                    } else {
                        // LOGIQUE PYTHON : seg = np.where(nonzero_mask, np.int8(0),
                        // np.int8(nonzero_label))
                        croppedSeg.data(0, x, y, z) = is_nonzero ? 0.0f : (float)nonzero_label;
                    }
                }
            }
        }

        return {croppedData, croppedSeg};
    }

    std::pair<NiftiVolume, std::vector<std::array<int, 2>>>
    Preprocessor::padVolume(const NiftiVolume &vol, int min_size) {
        const int C = vol.data.dimension(0);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);

        int newX = std::max(X, min_size);
        int newY = std::max(Y, min_size);
        int newZ = std::max(Z, min_size);

        std::vector<std::array<int, 2>> padding(3);
        int current_dims[3] = {X, Y, Z};
        int target_dims[3] = {newX, newY, newZ};

        for (int i = 0; i < 3; ++i) {
            int total_pad = target_dims[i] - current_dims[i];
            int pad_before = total_pad / 2;
            padding[i] = {pad_before, total_pad - pad_before};
        }

        NiftiVolume padded;
        padded.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(C, newX, newY, newZ);
        padded.data.setZero();
        padded.spacing = vol.spacing;

        for (int c = 0; c < C; ++c) {
            for (int x = 0; x < X; ++x) {
                for (int y = 0; y < Y; ++y) {
                    for (int z = 0; z < Z; ++z) {
                        padded.data(c, x + padding[0][0], y + padding[1][0], z + padding[2][0]) =
                            vol.data(c, x, y, z);
                    }
                }
            }
        }
        return {padded, padding};
    }

    QString Preprocessor::biasCorrect(const QString &input_path, const QString &prefix) {
        QString outDir = QFileInfo(input_path).absolutePath();
        QString cleanPrefix = QFileInfo(prefix).fileName();
        QString output_path = outDir + "/" + cleanPrefix + "_N4.nii.gz";

        QStringList args;
        args << "animaN4BiasCorrection" << "-i" << input_path << "-o" << output_path;

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

        // On extrait uniquement le nom du fichier du suffixe pour éviter de concaténer des chemins
        // absolus
        QString cleanSuffix = QFileInfo(prefix_label).fileName();

        // On construit le chemin de sortie dans le même dossier que l'input
        QString outDir = QFileInfo(input_path).absolutePath();
        const QString output_path = outDir + "/" + base_path_prefix + "_" + cleanSuffix + ".nii.gz";

        // On enlève le .nii.gz pour le fichier de transformation (.txt)
        QString trsf_base = output_path;
        if (trsf_base.endsWith(".nii.gz"))
            trsf_base.chop(7);
        const QString trsf_path = trsf_base + ".txt";

        QStringList args;
        args << "animaPyramidalBMRegistration"
             << "-r" << ref_path 
             << "-m" << input_path 
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

    PreprocessedVolume
    Preprocessor::preprocessModality(const QString &modality_path, bool is_MNI,
                                     std::array<std::array<int, 2>, 3> *bbox_ptr) {
        PreprocessedVolume result;

        // --- Step 1 & 2: Registration (Sortie en espace MNI Atlas) ---
        QString prefix =
            QFileInfo(modality_path).absolutePath() + "/" + QFileInfo(modality_path).baseName();
        if (prefix.endsWith(".nii"))
            prefix.chop(4);
        prefix += "_preproc";

        QString MNI_output;
        QString trsf_path;
        if (!is_MNI) {
            printAction("bias correction");
            MNI_output = biasCorrect(modality_path, prefix);

            printAction("register to MNI");
            std::tie(MNI_output, trsf_path) =
                registerToReference(MNI_output, atlasImage, prefix, "MNI");
        } else {
            MNI_output = modality_path;
            trsf_path.clear();
        }
        result.trsf_path = trsf_path;

        // --- Step 3: Load Nifti ---
        printAction("loading NIFTI volume");
        NiftiVolume vol = NiftiVolume::loadNifti(MNI_output);
        result.original_shape =
            Eigen::Vector3i(vol.data.dimension(1), vol.data.dimension(2), vol.data.dimension(3));

        float maxCheck = -1.0f;
        for (int i = 0; i < vol.data.size(); ++i)
            maxCheck = std::max(maxCheck, vol.data.data()[i]);
        spdlog::info("DEBUG: Vol loaded Max Intensity = {}", maxCheck);

        // --- Step 4: Crop (Retourne la paire {Image, Masque}) ---
        printAction("cropping");
        std::pair<NiftiVolume, NiftiVolume> cropped_pair;
        std::array<std::array<int, 2>, 3> computed_bbox;

        if (bbox_ptr && (*bbox_ptr)[0][0] != -1) {
            cropped_pair = cropToNonZero(vol, nullptr, -1, bbox_ptr);
        } else {
            cropped_pair = cropToNonZero(vol, nullptr, -1, &computed_bbox);
            if (bbox_ptr)
                *bbox_ptr = computed_bbox;
        }

        // --- Step 5: Resampling ---
        printAction("resampling to 1mm isotropic");
        Eigen::Vector3f target_spacing(1.0f, 1.0f, 1.0f);

        NiftiVolume resampled_img = resampler.resample(cropped_pair.first, target_spacing, false);
        NiftiVolume resampled_seg = resampler.resample(cropped_pair.second, target_spacing, true);

        // --- AJOUT DE SÉCURITÉ ICI ---
        // On s'assure que le masque ne contient que 0 ou -1 après le resampling
        float *segPtr = resampled_seg.data.data();
        for (int i = 0; i < resampled_seg.data.size(); ++i) {
            // Si la valeur est proche de 0, c'est le cerveau, sinon c'est le fond
            segPtr[i] = (segPtr[i] > -0.5f) ? 0.0f : -1.0f;
        }
        // -----------------------------

        // --- Step 6: Z-score normalization ---
        printAction("z-score normalization");
        zScoreNormalize(resampled_img, &resampled_seg);

        // --- Step 7: Padding ---
        printAction("padding");
        auto pad_result = padVolume(resampled_img, 128);

        // Stockage dans le résultat final
        result.data = pad_result.first.data;
        result.spacing = pad_result.first.spacing;
        result.padding = {pad_result.second[0], pad_result.second[1], pad_result.second[2]};

        // --- LOGS DE VERIFICATION ---
        float minVal = std::numeric_limits<float>::max();
        float maxVal = -std::numeric_limits<float>::max();
        double sum = 0;
        float *dataPtr = result.data.data();
        size_t sz = result.data.size();

        for (size_t i = 0; i < sz; ++i) {
            float v = dataPtr[i];
            minVal = std::min(minVal, v);
            maxVal = std::max(maxVal, v);
            sum += v;
        }

        spdlog::info(
            "FINAL STATS ({} voxels) - Dims: {}x{}x{} - Min: {:.4f}, Max: {:.4f}, Mean: {:.4f}", sz,
            result.data.dimension(1), result.data.dimension(2), result.data.dimension(3), minVal,
            maxVal, sum / (double)sz);

        if (config.get("keep_MNI", true).toBool()) {
            result.MNI_base_image = MNI_output;
        }

        return result;
    }
    
    PreprocessedVolume Preprocessor::preprocess(const QString &t1_path, const QString &flair_path,
                                                const QString &temp_dir, bool bet_only) {
        preprocessing_steps.clear();

        // 1. Préparation des noms de fichiers
        QFileInfo t1Info(t1_path);
        QString t1BaseName = t1Info.completeBaseName();
        if (t1BaseName.endsWith(".nii"))
            t1BaseName.chop(4);

        // Le préfixe pointe vers le dossier temporaire (output_data)
        QString prefix = temp_dir + "/" + t1BaseName;
        QString bet_t1_path;

        // --- Brain extraction T1 ---
        if (!t1BaseName.contains("BET") && !t1BaseName.contains("MNI")) {
            printAction("brain extraction");
            bet_t1_path = brainExtraction->run(t1_path, prefix);
        } else {
            bet_t1_path = temp_dir + "/" + t1Info.fileName();
            if (t1_path != bet_t1_path) {
                QFile::remove(bet_t1_path);
                QFile::copy(t1_path, bet_t1_path);
            }
        }

        PreprocessedVolume t1_result;
        if (!bet_only) {
            bool is_MNI = t1BaseName.contains("MNI");
            t1_result = preprocessModality(bet_t1_path, is_MNI);

            // --- Sauvegarde finale dans le dossier de sortie (temp_dir) ---
            if (config.get("save_preproc", true).toBool() && t1_result.data.size() > 0) {

                if (t1_result.data.dimension(1) != 128) {
                    spdlog::info("Final resize to 128x128x128 (current: {})",
                                 t1_result.data.dimension(1));
                }

                // Chemin de sortie : Dossier de build/output_data
                QString final_save_path = temp_dir + "/" + t1BaseName + "_MNI.nii.gz";
                spdlog::info("Saving normalized MNI volume to: {}", final_save_path.toStdString());

                NiftiVolume volToSave;
                volToSave.data = t1_result.data;
                volToSave.spacing = Eigen::Vector3f(1.0f, 1.0f, 1.0f);

                try {
                    if (!NiftiVolume::saveNifti(final_save_path, volToSave)) {
                        spdlog::error("Failed to save NIfTI to {}", final_save_path.toStdString());
                    }
                } catch (const std::exception &e) {
                    spdlog::error("Exception saving final MNI: {}", e.what());
                }
            }

            if (!is_MNI)
                moveToOutput(bet_t1_path);
        } else {
            moveToOutput(bet_t1_path);
        }

        // --- Option FLAIR ---
        if (!flair_path.isEmpty()) {
            QFileInfo flairInfo(flair_path);
            QString flairBaseName = flairInfo.completeBaseName();
            if (flairBaseName.endsWith(".nii"))
                flairBaseName.chop(4);

            QString flair_prefix = temp_dir + "/" + flairBaseName;
            QString bet_flair_path;

            if (!flairBaseName.contains("BET") && !flairBaseName.contains("MNI")) {
                printAction("register FLAIR to T1");
                auto [flair_registered, _] =
                    registerToReference(flair_path, t1_path, flair_prefix, "T1");
                preprocessing_steps.push_back(flair_registered);

                printAction("brain extraction");
                bet_flair_path = brainExtraction->run(flair_registered, flair_prefix);
            } else {
                bet_flair_path = temp_dir + "/" + flairInfo.fileName();
                if (flair_path != bet_flair_path) {
                    QFile::remove(bet_flair_path);
                    QFile::copy(flair_path, bet_flair_path);
                }
            }

            PreprocessedVolume flair_result;
            if (!bet_only) {
                bool is_MNI = flairBaseName.contains("MNI");
                // On réutilise le même padding que le T1 pour la cohérence
                flair_result = preprocessModality(bet_flair_path, is_MNI, &t1_result.padding);
                if (!is_MNI)
                    moveToOutput(bet_flair_path);
            } else {
                moveToOutput(bet_flair_path);
            }

            // --- Fusion des canaux (C, X, Y, Z) ---
            auto dims = t1_result.data.dimensions();
            int C_t1 = dims[0];
            int C_flair = flair_result.data.dimension(0);

            if (dims[1] != flair_result.data.dimension(1) ||
                dims[2] != flair_result.data.dimension(2) ||
                dims[3] != flair_result.data.dimension(3)) {
                throw std::runtime_error("T1 and FLAIR spatial dimensions mismatch");
            }

            Eigen::Tensor<float, 4, Eigen::RowMajor> combined(C_t1 + C_flair, dims[1], dims[2],
                                                              dims[3]);

            // Copie T1
            combined.slice(Eigen::array<Eigen::Index, 4>{0, 0, 0, 0},
                           Eigen::array<Eigen::Index, 4>{C_t1, dims[1], dims[2], dims[3]}) =
                t1_result.data;

            // Copie FLAIR
            combined.slice(Eigen::array<Eigen::Index, 4>{C_t1, 0, 0, 0},
                           Eigen::array<Eigen::Index, 4>{C_flair, dims[1], dims[2], dims[3]}) =
                flair_result.data;

            t1_result.data = combined;
        }

        // Déplacement des fichiers intermédiaires si demandé
        if (config.get("save_preproc", true).toBool()) {
            for (const auto &path : preprocessing_steps)
                moveToOutput(path);
        }

        return t1_result;
    }

    
    void Preprocessor::printAction(const QString &actionName) {
        spdlog::info("Starting {}...", actionName.toStdString());
    }

    QString Preprocessor::moveToOutput(const QString &img_path) {
        if (img_path.isEmpty() || !QFile::exists(img_path)) {
            return img_path;
        }
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

        if (QFileInfo(img_path).absoluteFilePath() == QFileInfo(dst).absoluteFilePath()) {
            return dst;
        }

        QFile::remove(dst);
        if (!QFile::copy(img_path, dst)) {
            // Au lieu de throw, on log une erreur pour ne pas stopper tout le pipeline
            spdlog::error("Failed to copy file from {} to {}", img_path.toStdString(),
                          dst.toStdString());
            return img_path;
        }

        return dst;
    }
    
} // namespace preprocessing