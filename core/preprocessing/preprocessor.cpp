#include "preprocessor.h"
#include <QtDebug>
#include <cassert>
#include <cmath>
#include <cstring>
#include <nifti1_io.h>
#include <spdlog/spdlog.h>

namespace preprocessing {

    void Preprocessor::zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg) {
        auto &tensor = vol.data;
        // Calcul de la moyenne
        Eigen::Tensor<float, 0, Eigen::ColMajor> meanTensor = tensor.mean();
        float mean = meanTensor(0);

        // Calcul de l'écart-type
        Eigen::Tensor<float, 0, Eigen::ColMajor> varTensor = (tensor - mean).square().mean();
        float std_dev = std::sqrt(std::max(varTensor(0), 1e-8f));

        // Assignation explicite
        tensor = (tensor - mean) / std_dev;
    }

    Eigen::Tensor<uint8_t, 3, Eigen::ColMajor>
    buildMask(const Eigen::Tensor<float, 4, Eigen::ColMajor> &data) {
        const int X = (int)data.dimension(0);
        const int Y = (int)data.dimension(1);
        const int Z = (int)data.dimension(2);

        float maxv = -1e9f;
        for (int x = 0; x < X; x++)
            for (int y = 0; y < Y; y++)
                for (int z = 0; z < Z; z++)
                    maxv = std::max(maxv, data(x, y, z, 0));

        float thr = 0.01f * maxv;
        Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> mask(X, Y, Z);

        for (int z = 0; z < Z; z++)
            for (int y = 0; y < Y; y++)
                for (int x = 0; x < X; x++)
                    mask(x, y, z) = (data(x, y, z, 0) > thr) ? (uint8_t)1 : (uint8_t)0;

        return mask;
    }

    std::array<std::array<int, 2>, 3>
    computeBBox(const Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> &mask) {
        int X = (int)mask.dimension(0);
        int Y = (int)mask.dimension(1);
        int Z = (int)mask.dimension(2);

        int xmin = X, xmax = -1, ymin = Y, ymax = -1, zmin = Z, zmax = -1;

        for (int z = 0; z < Z; z++)
            for (int y = 0; y < Y; y++)
                for (int x = 0; x < X; x++) {
                    if (mask(x, y, z)) {
                        xmin = std::min(xmin, x);
                        xmax = std::max(xmax, x);
                        ymin = std::min(ymin, y);
                        ymax = std::max(ymax, y);
                        zmin = std::min(zmin, z);
                        zmax = std::max(zmax, z);
                    }
                }

        if (xmax < xmin)
            throw std::runtime_error("Empty mask");
        return {{{xmin, xmax + 1}, {ymin, ymax + 1}, {zmin, zmax + 1}}};
    }

    std::pair<NiftiVolume, NiftiVolume>
    Preprocessor::cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg, int nonzero_label,
                                std::array<std::array<int, 2>, 3> *bbox_out) {
        auto mask = buildMask(vol.data);
        auto bbox = computeBBox(mask);
        if (bbox_out)
            *bbox_out = bbox;

        int x0 = bbox[0][0], nX = bbox[0][1] - x0;
        int y0 = bbox[1][0], nY = bbox[1][1] - y0;
        int z0 = bbox[2][0], nZ = bbox[2][1] - z0;
        int C = (int)vol.data.dimension(3);

        NiftiVolume cropped;
        cropped.spacing = vol.spacing;

        Eigen::array<Eigen::Index, 4> offsets = {x0, y0, z0, 0};
        Eigen::array<Eigen::Index, 4> extents = {nX, nY, nZ, C};

        Eigen::Tensor<float, 4, Eigen::ColMajor> temp = vol.data.slice(offsets, extents);
        cropped.data = temp;

        return {cropped, cropped};
    }

    std::pair<NiftiVolume, std::vector<std::array<int, 2>>>
    Preprocessor::padVolume(const NiftiVolume &vol, int min_size = 128, int div = 32) {
        const int X = (int)vol.data.dimension(0);
        const int Y = (int)vol.data.dimension(1);
        const int Z = (int)vol.data.dimension(2);
        const int C = (int)vol.data.dimension(3);

        auto get_target_size = [min_size, div](int current) {
            int base = std::max(current, min_size);
            return (base + div - 1) / div * div;
        };

        int tX = get_target_size(X);
        int tY = get_target_size(Y);
        int tZ = get_target_size(Z);

        int pX = (tX - X) / 2;
        int pY = (tY - Y) / 2;
        int pZ = (tZ - Z) / 2;

        Eigen::Tensor<float, 0> min_tensor = vol.data.minimum();
        float min_background = min_tensor(0);

        NiftiVolume padded;
        padded.spacing = vol.spacing;
        padded.data.resize(tX, tY, tZ, C);

        padded.data.setConstant(min_background);

        Eigen::array<Eigen::Index, 4> offsets = {pX, pY, pZ, 0};
        Eigen::array<Eigen::Index, 4> extents = {X, Y, Z, C};
        padded.data.slice(offsets, extents) = vol.data;

        std::vector<std::array<int, 2>> p_info = {
            {pX, tX - X - pX}, {pY, tY - Y - pY}, {pZ, tZ - Z - pZ}};

        return {padded, p_info};
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


        QString cleanSuffix = QFileInfo(prefix_label).fileName();

        QString outDir = QFileInfo(input_path).absolutePath();
        const QString output_path = outDir + "/" + base_path_prefix + "_" + cleanSuffix + ".nii.gz";

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
        QString path = modality_path;

        QString debug_prefix = QFileInfo(modality_path).absolutePath() + "/debug_" +
                               QFileInfo(modality_path).baseName();

        if (!is_MNI) {
            printAction("bias correction");
            QString prefix = QFileInfo(path).absolutePath() + "/" + QFileInfo(path).baseName();
            path = biasCorrect(path, prefix);

            printAction("registration to MNI atlas");
            auto [reg_path, trsf] = registerToReference(path, atlasImage, prefix, "MNI");
            path = reg_path;
            result.trsf_path = trsf;
        }

        printAction("loading NIFTI volume");
        NiftiVolume vol = NiftiVolume::loadNifti(path);
        NiftiVolume::saveNifti(debug_prefix + "_1_loaded.nii.gz", vol);

        printAction("cropping to non-zero content");
        std::array<std::array<int, 2>, 3> local_bbox = {{{-1, -1}, {-1, -1}, {-1, -1}}};
        auto [cropped, _] = cropToNonZero(vol, nullptr, -1, bbox_ptr ? bbox_ptr : &local_bbox);

        if (bbox_ptr && (*bbox_ptr)[0][0] == -1)
            *bbox_ptr = local_bbox;

        // Debug: Après crop
        NiftiVolume::saveNifti(debug_prefix + "_2_cropped.nii.gz", cropped);

        printAction("resampling to 1.0mm iso");
        NiftiVolume res = resampler.resample(cropped, Eigen::Vector3f(1.0f, 1.0f, 1.0f), false);

        // Debug: Après resampling
        NiftiVolume::saveNifti(debug_prefix + "_3_resampled.nii.gz", res);

        printAction("z-score normalization");
        zScoreNormalize(res);

        // Debug: Après normalization
        NiftiVolume::saveNifti(debug_prefix + "_4_normalized.nii.gz", res);

        printAction("padding to target size (128)");
        auto [padded, p_info] = padVolume(res, 128);

        // Debug: Volume final avant inference
        NiftiVolume::saveNifti(debug_prefix + "_5_final_padded.nii.gz", padded);

        result.data = padded.data;
        result.spacing = padded.spacing;
        result.padding = {p_info[0], p_info[1], p_info[2]};

        return result;
    }

    PreprocessedVolume Preprocessor::preprocess(const QString &t1_path, const QString &flair_path,
                                                const QString &temp_dir, bool bet_only) {
        QString bet_t1 = t1_path;
        if (!t1_path.contains("BET") && !t1_path.contains("MNI")) {
            bet_t1 = brainExtraction->run(t1_path, temp_dir + "/t1");
        }

        PreprocessedVolume t1_res = preprocessModality(bet_t1, t1_path.contains("MNI"));

        if (!flair_path.isEmpty()) {
            auto [fReg, _] = registerToReference(flair_path, t1_path, temp_dir + "/flair", "T1");
            QString fBet = brainExtraction->run(fReg, temp_dir + "/flair_bet");
            PreprocessedVolume f_res = preprocessModality(fBet, false, &t1_res.padding);

            auto d = t1_res.data.dimensions();
            int nC = (int)d[3] + (int)f_res.data.dimension(3);

            Eigen::Tensor<float, 4, Eigen::ColMajor> combined((int)d[0], (int)d[1], (int)d[2], nC);

            Eigen::array<Eigen::Index, 4> t1_ext = {(Eigen::Index)d[0], (Eigen::Index)d[1],
                                                    (Eigen::Index)d[2], (Eigen::Index)d[3]};
            combined.slice(Eigen::array<Eigen::Index, 4>{0, 0, 0, 0}, t1_ext) = t1_res.data;

            Eigen::array<Eigen::Index, 4> f_off = {0, 0, 0, (Eigen::Index)d[3]};
            Eigen::array<Eigen::Index, 4> f_ext = {(Eigen::Index)d[0], (Eigen::Index)d[1],
                                                   (Eigen::Index)d[2],
                                                   (Eigen::Index)f_res.data.dimension(3)};
            combined.slice(f_off, f_ext) = f_res.data;

            t1_res.data = combined;
        }

        return t1_res;
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