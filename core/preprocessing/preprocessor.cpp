#include "preprocessor.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <nifti1_io.h>

namespace preprocessing {

    void Preprocessor::zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg) {
        auto &tensor = vol.data;

        // 1. Calcul de la moyenne
        Eigen::Tensor<float, 0, Eigen::RowMajor> meanTensor = tensor.mean();

        // 2. Extraction de la valeur scalaire
        float mean = meanTensor(0); // Le second () extrait la valeur du tenseur 0D

        // 2. Calcul de l'écart-type : sqrt(mean( (x - mean)^2 ))
        Eigen::Tensor<float, 0, Eigen::RowMajor> variance = (tensor - mean).square().mean();
        float std_dev = std::sqrt(std::max(variance(), 1e-8f));

        // 3. Application (Opération vectorisée en une ligne)
        tensor = (tensor - mean) / std_dev;
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
    Preprocessor::cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg, int nonzero_label,
                                std::array<std::array<int, 2>, 3> *bbox_out) {
        const auto &data = vol.data;
        const int X = data.dimension(1);
        const int Y = data.dimension(2);
        const int Z = data.dimension(3);

        std::array<std::array<int, 2>, 3> bbox;

        // 1. Calcul de la Bounding Box (si non fournie)
        if (bbox_out && (*bbox_out)[0][0] != -1) {
            bbox = *bbox_out;
        } else {
            // On crée un masque booléen : vrai si au moins un canal a une valeur > epsilon
            // .abs() > 1e-5f renvoie un tenseur de booléens
            // .any(Eigen::array<int, 1>{0}) réduit la dimension des canaux (C)
            Eigen::Tensor<bool, 3, Eigen::RowMajor> mask =
                (data.abs() > 1e-5f).any(Eigen::array<int, 1>{0});

            auto get_limits = [&](int dim) -> std::pair<int, int> {
                // On réduit les deux autres dimensions pour ne garder que celle qui nous intéresse
                Eigen::array<int, 2> dims_to_reduce;
                if (dim == 0)
                    dims_to_reduce = {1, 2}; // Pour X, on réduit Y et Z
                else if (dim == 1)
                    dims_to_reduce = {0, 2}; // Pour Y, on réduit X et Z
                else
                    dims_to_reduce = {0, 1}; // Pour Z, on réduit X et Y

                Eigen::Tensor<bool, 1, Eigen::RowMajor> projection = mask.any(dims_to_reduce);

                int min_idx = -1, max_idx = -1;
                for (int i = 0; i < projection.size(); ++i) {
                    if (projection(i)) {
                        if (min_idx == -1)
                            min_idx = i;
                        max_idx = i;
                    }
                }
                if (min_idx == -1)
                    throw std::runtime_error("All-zero volume, cannot crop");
                return {min_idx, max_idx + 1};
            };

            bbox[0] = {get_limits(0).first, get_limits(0).second};
            bbox[1] = {get_limits(1).first, get_limits(1).second};
            bbox[2] = {get_limits(2).first, get_limits(2).second};
        }

        if (bbox_out)
            *bbox_out = bbox;

        // 2. Extraction par Slicing (Ultra rapide)
        int newX = bbox[0][1] - bbox[0][0];
        int newY = bbox[1][1] - bbox[1][0];
        int newZ = bbox[2][1] - bbox[2][0];

        Eigen::array<Eigen::Index, 4> offsets = {0, bbox[0][0], bbox[1][0], bbox[2][0]};
        Eigen::array<Eigen::Index, 4> extents = {(Eigen::Index)data.dimension(0), newX, newY, newZ};

        NiftiVolume croppedData;
        croppedData.spacing = vol.spacing;
        croppedData.data = data.slice(offsets, extents);

        // 3. Gestion du masque de segmentation
        NiftiVolume croppedSeg;
        croppedSeg.spacing = vol.spacing;

        if (seg) {
            croppedSeg.data = seg->data.slice(offsets, extents);
        } else {
            Eigen::array<int, 1> reduction_axis = {0};
            Eigen::Tensor<bool, 3, Eigen::RowMajor> local_mask =
                (croppedData.data.abs() > 1e-5f).any(reduction_axis);

            // 2. Préparer les constantes au format FLOAT explicitement
            float f_zero = 0.0f;
            float f_label = static_cast<float>(nonzero_label);

            // 3. Utiliser select avec des types strictement identiques (float)
            // On caste local_mask en float AVANT de faire les opérations si nécessaire,
            // ou on s'assure que select renvoie des float.
            croppedSeg.data.chip(0, 0) =
                local_mask.select(croppedSeg.data.chip(0, 0).constant(f_zero),
                                  croppedSeg.data.chip(0, 0).constant(f_label));
        }

        return {croppedData, croppedSeg};
    }

    std::pair<NiftiVolume, std::vector<std::array<int, 2>>>
    Preprocessor::padVolume(const NiftiVolume &vol, int min_size) {
        const int C = vol.data.dimension(0);
        const int X = vol.data.dimension(1);
        const int Y = vol.data.dimension(2);
        const int Z = vol.data.dimension(3);

        // Calcul des paddings (même logique)
        int padX = std::max(0, min_size - X);
        int padY = std::max(0, min_size - Y);
        int padZ = std::max(0, min_size - Z);

        std::vector<std::array<int, 2>> padding = {{padX / 2, padX - (padX / 2)},
                                                   {padY / 2, padY - (padY / 2)},
                                                   {padZ / 2, padZ - (padZ / 2)}};

        NiftiVolume padded;
        padded.spacing = vol.spacing;

        // Eigen padding : on définit les paires de (avant, après) pour chaque dimension
        Eigen::array<std::pair<int, int>, 4> pad_dims;
        pad_dims[0] = {0, 0}; // Pas de pad sur les canaux (C)
        pad_dims[1] = {padding[0][0], padding[0][1]};
        pad_dims[2] = {padding[1][0], padding[1][1]};
        pad_dims[3] = {padding[2][0], padding[2][1]};

        // L'opération magique :
        padded.data = vol.data.pad(pad_dims);

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