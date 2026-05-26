#pragma once

#include <Eigen/Core>
#include <unsupported/Eigen/CXX11/Tensor>

#include <array>
#include <string>
#include <vector>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

struct PreprocessedVolume 
{
    /**
     * @brief Volume data (C, X, Y, Z)
     */
    Eigen::Tensor<float, 4, Eigen::ColMajor> data;

    /**
     * @brief Affine transformation matrix (voxel → world)
     */
    Eigen::Matrix4f affine = Eigen::Matrix4f::Identity();

    /**
     * @brief Original image shape before preprocessing (X, Y, Z)
     */
    Eigen::Vector3i original_shape;

    /**
     * @brief Path to the original T1 image (before any preprocessing)
     */
    QString original_t1_path;

    /**
     * @brief Path to the deformation / transformation file
     */
    QString trsf_path;

    /**
     * @brief Voxel spacing (sx, sy, sz)
     */
    Eigen::Vector3f spacing;

    /**
     * @brief Padding applied on each axis: {{x0, x1}, {y0, y1}, {z0, z1}}
     */
    std::array<std::array<int, 2>, 3> padding{{{0, 0}, {0, 0}, {0, 0}}};

    /**
    * @brief Bounding box of the brain in the original image: {{x0, x1}, {y0, y1}, {z0, z1}}
    * This is used for cropping the post-processed segmentation back to the original space.
    */
    std::array<std::array<int, 2>, 3> bbox{{{0, 0}, {0, 0}, {0, 0}}};

    /**
     * @brief Reference MNI image used for registration
     */
    QString MNI_base_image;


    void saveMetadata(const QString &path) {
        QJsonObject obj;

        // paths
        obj["original_t1_path"] = this->original_t1_path;
        obj["trsf_path"] = this->trsf_path;
        obj["MNI_base_image"] = this->MNI_base_image;

        // Spacing (Eigen::Vector3f)
        QJsonArray spacing;
        spacing.append(this->spacing.x());
        spacing.append(this->spacing.y());
        spacing.append(this->spacing.z());
        obj["spacing"] = spacing;

        // Original Shape (Eigen::Vector3i)
        QJsonArray shape;
        shape.append(this->original_shape.x());
        shape.append(this->original_shape.y());
        shape.append(this->original_shape.z());
        obj["original_shape"] = shape;

        // Bounding Box (std::array<std::array<int, 2>, 3>)
        QJsonArray bbox;
        for (int i = 0; i < 3; ++i) {
            bbox.append(this->bbox[i][0]);
            bbox.append(this->bbox[i][1]);
        }
        obj["bbox"] = bbox;

        // Padding (std::array<std::array<int, 2>, 3>)
        QJsonArray padding;
        for (int i = 0; i < 3; ++i) {
            padding.append(this->padding[i][0]);
            padding.append(this->padding[i][1]);
        }
        obj["padding"] = padding;

        QJsonDocument doc(obj);
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(doc.toJson());
            file.close();
            qDebug() << "[CACHE] Metadata saved to:" << path;
        }
    }

    bool loadMetadata(const QString &path) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return false;

        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject obj = doc.object();

        // paths
        this->original_t1_path = obj["original_t1_path"].toString();
        this->trsf_path = obj["trsf_path"].toString();
        this->MNI_base_image = obj["MNI_base_image"].toString();

        // Spacing Reconstruction
        QJsonArray spacingArr = obj["spacing"].toArray();
        this->spacing = Eigen::Vector3f(spacingArr[0].toDouble(), spacingArr[1].toDouble(),
                                      spacingArr[2].toDouble());

        // Original Shape Reconstruction
        QJsonArray shapeArr = obj["original_shape"].toArray();
        this->original_shape =
            Eigen::Vector3i(shapeArr[0].toInt(), shapeArr[1].toInt(), shapeArr[2].toInt());

        // BBox Reconstruction
        QJsonArray bboxArr = obj["bbox"].toArray();
        this->bbox[0][0] = bboxArr[0].toInt();
        this->bbox[0][1] = bboxArr[1].toInt();
        this->bbox[1][0] = bboxArr[2].toInt();
        this->bbox[1][1] = bboxArr[3].toInt();
        this->bbox[2][0] = bboxArr[4].toInt();
        this->bbox[2][1] = bboxArr[5].toInt();

        // Padding Reconstruction
        QJsonArray padArr = obj["padding"].toArray();
        this->padding[0][0] = padArr[0].toInt();
        this->padding[0][1] = padArr[1].toInt();
        this->padding[1][0] = padArr[2].toInt();
        this->padding[1][1] = padArr[3].toInt();
        this->padding[2][0] = padArr[4].toInt();
        this->padding[2][1] = padArr[5].toInt();

        file.close();
        qDebug() << "[CACHE] Metadata successfully loaded.";
        return true;
    }
};