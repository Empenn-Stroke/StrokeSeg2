#pragma once

#include <QString>
#include <QStringList>

#include "utils/animawrapper.h"
#include "utils/path.h"

#include <Qdir>
#include <stdexcept>

#ifndef CORE_PREPROCESSING_BRAINEXTRACTION_H
#define CORE_PREPROCESSING_BRAINEXTRACTION_H


class AnimaWrapper;
class MainWindow; // GUI

/**
 * @class BrainExtraction
 * @brief This class handle the brain extraction during the preprocessing
 */
class BrainExtraction : public QObject {
    Q_OBJECT
    public:
        /**
        * @brief Constructor of the brain extraction class
        * @param wrapper(AnimaWrapper): A wrapper to simplify the use of anima executables atlasImage
        * @param atlasImage(Qstring): _description_
        * @param gui(MainWindow): _description_. Optional, default to null
        */
      BrainExtraction(AnimaWrapper *wrapper, const QString &atlasImage, QObject *parent = nullptr);


        /**
        * @brief Performs the brain extraction on 3D image. Composed by a sequence of anima commands. Store all intermediate results in the temporary directory
        * @param imgPath(QString): Input path 
        * @param prefix(QString): Composed of the temporary folder path and the input file’s base name without its extension
        * @return (QString): Path of the brain extracted image
        */
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

        /**
        * @brief Runs the command and make an exception if the user cancelled the action.
        * @param command(QStringList): A specific command
        */
      void runCommand(const QStringList &command);
};

#endif