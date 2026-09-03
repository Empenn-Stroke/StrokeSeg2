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
 * @brief This class handle the brain extraction during the preprocessing
 */
class BrainExtractor : public QObject 
{
    Q_OBJECT
    public:
      BrainExtractor(AnimaWrapper *wrapper, const QString &atlasImage, QObject *parent = nullptr);

      QString run(const QString &imgPath, const QString &prefix);

      QString runWithExistingTransform(const QString &imgPath, const QString &prefix, const QString &anchorImgPath, const QString &anchorAffTrsfPath, const QString &anchorNlTrsfPath);

      /**
       * @brief Exposes the atlas image this extractor is configured with, so callers can
       * pick the right BrainExtractor instance per reference family (T1 vs T2).
       */
      QString atlasImage() const { return m_atlasImage; }

    public slots:
      void requestCancel();

    signals:
      void progress(float value, const QString &message);
      void finished(const QString &outputPath);
      void error(const QString &message);


    private: 
      AnimaWrapper* m_wrapper;
      QString m_atlasImage;
      QString m_iccImage;
      QStringList m_pyramidOption;

      
      bool m_cancelRequested = false;

    private:
      void runCommand(const QStringList &command);
};
