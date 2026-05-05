#include "preprocessor.h"


#include <cassert>
#include <cmath>
#include <cstring>
#include <nifti1_io.h>

#include <qelapsedtimer.h>
#include <QtDebug>

#include "managers/progressManager.h"

namespace preprocessing {
    
    /**
     * @brief Applies z-score normalization to the input volume.
     * Compute mean and standard deviation to transform intensities : 
     * $z = \frac{x - \mu}{\sigma}$.
     * @param vol Volume to normalize. The operation is performed in-place.
     * @param seg Optional segmentation mask. If provided, only voxels corresponding to the
     * specified label (nonzero_label) will be considered for mean and standard deviation
     * calculation.
     */
    void Preprocessor::zScoreNormalize(NiftiVolume &vol, const NiftiVolume *seg) 
    {
        auto &tensor = vol.data;
        // mean calculation
        Eigen::Tensor<float, 0, Eigen::ColMajor> meanTensor = tensor.mean();
        float mean = meanTensor(0);

        // standard deviation calculation
        Eigen::Tensor<float, 0, Eigen::ColMajor> varTensor = (tensor - mean).square().mean();
        float std_dev = std::sqrt(std::max(varTensor(0), 1e-8f));

        // explicit assignment
        tensor = (tensor - mean) / std_dev;
    }

    /**
     * @brief Identifies non-zero voxels in the input volume to create a binary mask of active
     * regions.
     * @param vol Input volume
     * @return output binary mask where voxels with intensity above a certain threshold (1% of the
     * maximum
     */
    Eigen::Tensor<uint8_t, 3, Eigen::ColMajor>
    Preprocessor::buildMask(const Eigen::Tensor<float, 4, Eigen::ColMajor> &data) 
    {
        const int X = (int)data.dimension(0);
        const int Y = (int)data.dimension(1);
        const int Z = (int)data.dimension(2);

        Eigen::Tensor<float, 0> maxAsTensor = data.maximum();
        float maxv = maxAsTensor(0);

        float thr = 0.01f * maxv;
        Eigen::Tensor<uint8_t, 3, Eigen::ColMajor>  mask = (data.chip(0, 3) > thr).cast<uint8_t>();

        return mask;
    }

    /*
     * @brief Compute the bounding box of the non-zero region in the binary mask.
     * @param mask Binary mask indicating active regions (non-zero voxels).
     * @return Array of pairs representing the minimum and maximum indices along each dimension (X,
     * Y, Z)
    */
    std::array<std::array<int, 2>, 3>
    Preprocessor::computeBBox(const Eigen::Tensor<uint8_t, 3, Eigen::ColMajor> &mask) 
    {
        auto x_any = mask.any(Eigen::array<int, 2>{1, 2});
        auto y_any = mask.any(Eigen::array<int, 2>{0, 2});
        auto z_any = mask.any(Eigen::array<int, 2>{0, 1});

        auto get_range = [](const Eigen::Tensor<bool, 1, Eigen::ColMajor> &any_tensor,
                            int size) -> std::array<int, 2> 
        {
            int min_idx = 0;
            int max_idx = size - 1;
            while (min_idx < size && !any_tensor(min_idx))
                min_idx++;
            while (max_idx >= 0 && !any_tensor(max_idx))
                max_idx--;
            if (min_idx > max_idx)
                return {0, 0};

            return {min_idx, max_idx + 1};
        };

        auto rx = get_range(x_any, (int)mask.dimension(0));
        auto ry = get_range(y_any, (int)mask.dimension(1));
        auto rz = get_range(z_any, (int)mask.dimension(2));

        if (rx[1] == 0)
            throw std::runtime_error("Empty mask");
        return {rx, ry, rz};
    }

    /**
     * @brief Reduce the volume to the bounding box of non-zero voxels, effectively cropping out
     * irrelevant background.
     * @param vol Source volume to crop. The operation is performed in-place.
     * @param seg Segmentation volume corresponding to the input volume. If provided, it will be
     * cropped
     * @param nonzero_label Label in the segmentation to consider as "non-zero" for cropping. Only
     * voxels with this label
     * @param bbox_out Optional output parameter to receive the bounding box coordinates used for
     * cropping.
     * @return Pair containing the cropped volume and the corresponding cropped segmentation
     */
    std::pair<NiftiVolume, NiftiVolume>
    Preprocessor::cropToNonZero(const NiftiVolume &vol, const NiftiVolume *seg, int nonzero_label,
                                std::array<std::array<int, 2>, 3> *bbox_out) 
    {
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

        cropped.data = vol.data.slice(offsets, extents);

        return {cropped, cropped};
    }

    /**
     * @brief Adds padding to the volume to fit a minimum size and a multiple, requirements of the inference.
     * @param vol Input volume.
     * @param min_size Minimal size required for inference.
     * @param div Divisibility factor required for inference.
     * @return Pair containing padded volume and padding offsets.
     */
    std::pair<NiftiVolume, std::vector<std::array<int, 2>>>
    Preprocessor::padVolume(const NiftiVolume &vol, int min_size = 128, int div = 32) 
    {
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
            {pX, X + pX}, {pY, Y + pY}, {pZ, Z + pZ}};

        return {padded, p_info};
    }

    /**
     * @brief Correct inhomogeneity luminance fields in the volume.
     * @param input_path input volume file path.
     * @param prefix output volume file prefix.
     * @return Output volume file path.
     */
    QString Preprocessor::biasCorrect(const QString &input_path, const QString &prefix) 
    {
        QString outDir = QFileInfo(input_path).absolutePath();
        QString cleanPrefix = QFileInfo(prefix).fileName();
        QString output_path = outDir + "/" + cleanPrefix + "_N4.nii.gz";

        QStringList args;
        args << "animaN4BiasCorrection" << "-i" << input_path << "-o" << output_path;

        int ret = m_wrapper->run(args);
        if (ret != 0) {
            std::string err_msg = m_wrapper->lastStderr().toStdString();
            if (err_msg.empty())
                err_msg = "Unknown error in AnimaWrapper";
            throw std::runtime_error("Bias correction failed: " + err_msg);
        }

        return output_path;
    }

    /**
     * @brief Spatially aligns volume over a reference atlas (MNI)
     * @param input_path Volume path to register.
     * @param mni_image_path reference target (atlas).
     * @param base_path_prefix Workspace directory.
     * @param prefix_label Label to identify output file.
     * @return Pair including [Path to registered image, Path to transformation matrix].
     */
    std::pair<QString, QString> Preprocessor::registerToReference(const QString &input_path,
                                                                  const QString &ref_path,
                                                                  const QString &base_path_prefix,
                                                                  const QString &prefix_label) 
    {


        QString cleanSuffix = QFileInfo(base_path_prefix).fileName();

        QString outDir = QFileInfo(input_path).absolutePath();
        const QString output_path = outDir + "/" + prefix_label + "_" + cleanSuffix + ".nii.gz";

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

        int ret = m_wrapper->run(args);
        if (ret != 0) {
            std::string err_msg = m_wrapper->lastStderr().toStdString();
            if (err_msg.empty())
                err_msg = "Unknown error in AnimaWrapper";
            throw std::runtime_error("Bias correction failed: " + err_msg);
        }

        return std::pair<QString, QString>(output_path, trsf_path);
    }

    /**
     * @brief Performs the complete preprocessing pipeline on a single modality, including bias
     * correction, registration to MNI space, cropping, resampling, normalization, and padding.
     * @param modality_path Path to the input image modality (e.g., T1 or FLAIR).
     * @param is_MNI Defines if the input image is already in MNI space, in which case bias
     * correction and registration
     * @param bbox_ptr Coordinates of the bounding box used for cropping. If provided, it will be
     * filled with the coordinates of the cropping box. If nullptr, the cropping box will be
     * computed but not returned.
     * @return PreprocessedVolume for this modality.
     */
    PreprocessedVolume
    Preprocessor::preprocessModality(const QString &modality_path, bool is_MNI,
                                     std::array<std::array<int, 2>, 3> *bbox_ptr) 
    {
        PreprocessedVolume result;
        QString path = modality_path;

        result.original_t1_path = modality_path;

        qDebug() << "Preprocessing modality:" << modality_path;

        QString debug_prefix =
            QFileInfo(modality_path).absolutePath() + "/" +
                               QFileInfo(modality_path).baseName();

        if (!is_MNI) {
            ProgressManager::instance().report(41, 9, 10,
                                               new QString("Registering to MNI space"));

            printAction("bias correction");
            QString prefix = QFileInfo(path).absolutePath() + "/" + QFileInfo(path).baseName();
            path = biasCorrect(path, prefix);

            printAction("registration to MNI atlas");
            auto [reg_path, trsf] = registerToReference(path, m_atlasImage, prefix, "MNI");
            path = reg_path;
            result.trsf_path = trsf;

            qDebug() << "trsf path:" << trsf;
        }

        ProgressManager::instance().report(41, 9, 20, new QString("Cropping to non-zero content"));

        printAction("loading NIFTI volume");
        NiftiVolume vol = NiftiVolume::loadNifti(path);

        if (m_save_intermediary_steps) {
            NiftiVolume::saveNifti(debug_prefix + "_loaded.nii.gz", vol);
        }

        result.original_shape = Eigen::Vector3i((int)vol.data.dimension(0), (int)vol.data.dimension(1), (int)vol.data.dimension(2));

        printAction("cropping to non-zero content");
        std::array<std::array<int, 2>, 3> local_bbox = {{{-1, -1}, {-1, -1}, {-1, -1}}};
        auto [cropped, _] = cropToNonZero(vol, nullptr, -1, bbox_ptr ? bbox_ptr : &local_bbox);

        ProgressManager::instance().report(41, 9, 40);

        if (bbox_ptr && (*bbox_ptr)[0][0] == -1)
            *bbox_ptr = local_bbox;

        // --- LOG DE LA BBOX ---
        auto &b = bbox_ptr ? *bbox_ptr : local_bbox;
        qInfo().noquote() << QString(
                                 "[BBOX] X: [%1, %2], Y: [%3, %4], Z: [%5, %6] (Size: %7x%8x%9)")
                                 .arg(b[0][0])
                                 .arg(b[0][1])
                                 .arg(b[1][0])
                                 .arg(b[1][1])
                                 .arg(b[2][0])
                                 .arg(b[2][1])
                                 .arg(b[0][1] - b[0][0])
                                 .arg(b[1][1] - b[1][0])
                                 .arg(b[2][1] - b[2][0]);

        result.bbox = local_bbox;

        // Debug: Après crop
        if (m_save_intermediary_steps) {
            NiftiVolume::saveNifti(debug_prefix + "_cropped.nii.gz", cropped);
        }

        ProgressManager::instance().report(41, 9, 80, new QString("Resampling to 1.0mm iso"));

        printAction("resampling to 1.0mm iso");
        NiftiVolume res = m_resampler.resample(cropped, Eigen::Vector3f(1.0f, 1.0f, 1.0f), false);

        // Debug: Après resampling
        if (m_save_intermediary_steps) {
            NiftiVolume::saveNifti(debug_prefix + "_resampled.nii.gz", res);
        }

        ProgressManager::instance().report(41, 9, 85, new QString("Z-score normalization"));

        printAction("z-score normalization");
        zScoreNormalize(res);

        // Debug: Après normalization
        if (m_save_intermediary_steps) {
            NiftiVolume::saveNifti(debug_prefix + "_normalized.nii.gz", res);
        }

        ProgressManager::instance().report(41, 9, 90, new QString("Padding to minimum size 128 and multiple of 32"));

        printAction("padding to target size (128)");
        auto [padded, p_info] = padVolume(res, 128);

        qInfo().noquote()
            << QString("[PADDING] X: [low:%1, high:%2], Y: [low:%3, high:%4], Z: [low:%5, high:%6]")
                   .arg(p_info[0][0])
                   .arg(p_info[0][1])
                   .arg(p_info[1][0])
                   .arg(p_info[1][1])
                   .arg(p_info[2][0])
                   .arg(p_info[2][1]);

        qInfo().noquote() << QString("[FINAL SHAPE] %1x%2x%3")
                                 .arg(padded.data.dimension(0))
                                 .arg(padded.data.dimension(1))
                                 .arg(padded.data.dimension(2));
        // Volume final avant inference
        NiftiVolume::saveNifti(debug_prefix + "_PREPROC.nii.gz", padded);

        ProgressManager::instance().report(41, 9, 100);

        result.data = padded.data;
        result.spacing = padded.spacing;
        result.padding = {p_info[0], p_info[1], p_info[2]};

        return result;
    }

    /**
     * @brief Exécute le pipeline complet sur une paire de modalités (T1 et FLAIR).
     * @param t1_path Chemin vers l'image T1.
     * @param flair_path Chemin vers l'image FLAIR.
     * @param temp_dir Répertoire temporaire pour les fichiers intermédiaires.
     * @param bet_only Si vrai, arrête le traitement après l'extraction du cerveau.
     * @return PreprocessedVolume Objet contenant les volumes finaux et leurs métadonnées.
     */
    PreprocessedVolume Preprocessor::preprocess(const QString &t1_path, const QString &flair_path,
                                                const QString &temp_dir, bool bet_only, bool mni) 
    {
        QElapsedTimer bet_timer;
        bet_timer.start();
        QString bet_t1 = t1_path;
        if (!t1_path.contains("BET") && !t1_path.contains("MNI")) {
            connect(m_brainExtraction, &BrainExtraction::progress, [](float value, const QString &message) {
                ProgressManager::instance().report(0, 41, 10 + (int)(value * 30), new QString(message));
            });

            bet_t1 = m_brainExtraction->run(t1_path, temp_dir + "/" + QFileInfo(t1_path).baseName());
        }

        qDebug() << "Brain extraction took" << bet_timer.elapsed() / 1000 << "s";

        if (bet_only) {
            PreprocessedVolume res;

            if (!mni) {
                ProgressManager::instance().report(41, 9, 10,
                                                   new QString("Registering to MNI space"));

                printAction("bias correction");
                QString prefix =
                    QFileInfo(bet_t1).absolutePath() + "/" + QFileInfo(bet_t1).baseName();
                bet_t1 = biasCorrect(bet_t1, prefix);

                printAction("registration to MNI atlas");
                auto [reg_path, trsf] = registerToReference(bet_t1, m_atlasImage, prefix, "MNI");
                bet_t1 = reg_path;
                res.trsf_path = trsf;

                qDebug() << "trsf path:" << trsf;
            }

            res.original_t1_path = bet_t1;

            return res;
        }

        qDebug() << "Brain extraction took" << bet_timer.elapsed() / 1000 << "s";

        PreprocessedVolume t1_res = preprocessModality(bet_t1, t1_path.contains("MNI"));

        if (!flair_path.isEmpty()) {
            auto [fReg, _] = registerToReference(flair_path, t1_path, temp_dir + "/flair", "T1");
            QString fBet = m_brainExtraction->run(fReg, temp_dir + "/flair_bet");
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
        qDebug() << "Starting" << actionName << "...";
    }

    /**
     * @brief Déplace le fichier final vers le répertoire de sortie définitif.
     * @param img_path Chemin actuel du fichier.
     * @return Nouveau chemin du fichier.
     */
    QString Preprocessor::moveToOutput(const QString &img_path) 
    {
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
            qCritical() << "Failed to copy file from" << img_path << "to" << dst;
            return img_path;
        }

        return dst;
    }

} // namespace preprocessing