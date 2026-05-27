#pragma once

#include <QString>
#include <QStringList>

#include "utils/animawrapper.h"
#include "utils/env_path.h"

#include <QDir>
#include <stdexcept>

#ifndef CORE_PREPROCESSING_BRAINEXTRACTION_H
#define CORE_PREPROCESSING_BRAINEXTRACTION_H


class AnimaWrapper;
class MainWindow;

/**
 * @class BrainExtraction
 * @brief This class handle the brain extraction during the preprocessing
 */
class BrainExtraction : public QObject 
{
    Q_OBJECT
    public:
      BrainExtraction(AnimaWrapper *wrapper, const QString &atlasImage, QObject *parent = nullptr);

      QString run(const QString &imgPath, const QString &prefix);

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

#endif