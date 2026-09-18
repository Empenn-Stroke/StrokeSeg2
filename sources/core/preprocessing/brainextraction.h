// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

#include "utils/animawrapper.h"
#include "utils/env_path.h"

#include <QDir>
#include <stdexcept>

class AnimaWrapper;
class MainWindow;

/**
 * @class BrainExtractor
 * @brief This class handles the brain extraction during the preprocessing.
 */
class BrainExtractor : public QObject 
{
    Q_OBJECT
public:
    /**
     * @brief Constructor for the BrainExtractor class.
     *
     * @param wrapper A pointer to the AnimaWrapper object.
     * @param atlasImage The path to the atlas image used for brain extraction.
     * @param parent The parent QObject.
     */
    BrainExtractor(AnimaWrapper *wrapper, const QString &atlasImage, QObject *parent = nullptr);

    /**
     * @brief Run brain extraction on a given image.
     *
     * @param imgPath The path to the input image.
     * @param prefix The prefix for the output files.
     * @return The path to the output file.
     */
    QString run(const QString &imgPath, const QString &prefix);

    /**
     * @brief Run brain extraction on a given image using an existing transformation.
     *
     * @param imgPath The path to the input image.
     * @param prefix The prefix for the output files.
     * @param anchorImgPath The path to the anchor image.
     * @param anchorAffTrsfPath The path to the affine transformation file.
     * @param anchorNlTrsfPath The path to the non-linear transformation file.
     * @return The path to the output file.
     */
    QString runWithExistingTransform(const QString &imgPath, const QString &prefix, const QString &anchorImgPath, const QString &anchorAffTrsfPath, const QString &anchorNlTrsfPath);

    /**
     * @brief Exposes the atlas image this extractor is configured with, so callers can
     * pick the right BrainExtractor instance per reference family (T1 vs T2).
     *
     * @return The path to the atlas image.
     */
    QString atlasImage() const { return m_atlasImage; }

public slots:
    /**
     * @brief Slot to request cancellation of the current brain extraction process.
     */
    void requestCancel();

signals:
    /**
     * @brief Signal emitted to indicate progress during the brain extraction process.
     *
     * @param value The progress value as a float.
     * @param message The progress message as a QString.
     */
    void progress(float value, const QString &message);

    /**
     * @brief Signal emitted to indicate that the brain extraction process has finished.
     *
     * @param outputPath The path to the output file.
     */
    void finished(const QString &outputPath);

    /**
     * @brief Signal emitted to indicate an error during the brain extraction process.
     *
     * @param message The error message as a QString.
     */
    void error(const QString &message);

private:
    /**
     * @brief Run a command using the AnimaWrapper.
     *
     * @param command The command to be run as a QStringList.
     */
    void runCommand(const QStringList &command);

private: 
    AnimaWrapper* m_wrapper;
    QString m_atlasImage;
    QString m_iccImage;
    QStringList m_pyramidOption;

    bool m_cancelRequested = false;
};
