#include "brainextraction.h"

#include <stdexcept>

#include <QElapsedTimer>

#include "managers/progressManager.h"

/**
 * @brief Constructor of the brain extraction class
 * @param wrapper(AnimaWrapper): A wrapper to simplify the use of anima executables atlasImage
 * @param atlasImage(Qstring): Path of the atlas image used for the registration. The atlas
 * image should be in the same space as the input image (e.g. MNI space). It will be used as a
 * reference for the registration and the brain mask creation. The atlas image should be a 3D
 * image with a brain mask (e.g. BrainMask.nrrd) in the same directory.
 *
 */
BrainExtraction::BrainExtraction(AnimaWrapper *wrapper, const QString &atlasImage, QObject *parent)
    : QObject(parent), m_wrapper(wrapper), m_atlasImage(atlasImage),
      m_iccImage(QDir(Paths::atlasDir()).filePath("BrainMask.nrrd")),
      m_pyramidOption({"-p", "4", "-l", "1"}) {}

void BrainExtraction::requestCancel() 
{
    m_cancelRequested = true;
}

/**
 * @brief Runs the command and make an exception if the user cancelled the action.
 * @param command(QStringList): A specific command
 */
void BrainExtraction::runCommand(const QStringList &command) 
{
    if (m_cancelRequested) {
        throw std::runtime_error("Brain extraction cancelled by user");
    }

    int exitCode = m_wrapper->run(command);

    if (exitCode != 0) {
        QString toolName = command.isEmpty() ? "Unknown tool" : command.first();
        throw std::runtime_error(QString("%1 failed with exit code %2")
                                 .arg(toolName)
                                 .arg(exitCode).toStdString());
    }
}

/**
 * @brief Performs the brain extraction on 3D image. Composed by a sequence of anima commands. Store
 * all intermediate results in the temporary directory
 * @param imgPath(QString): Input path
 * @param prefix(QString): Composed of the temporary folder path and the input file’s base name
 * without its extension
 * @return (QString): Path of the brain extracted image
 */
QString BrainExtraction::run(const QString &imgPath, const QString &prefix) 
{
    try {
        QElapsedTimer timer;
        timer.start();
        emit progress(0.0f, "Starting brain extraction");

        QString brainMask = prefix + "_brainMask.nii.gz";
        QString maskedBrain = prefix + "_BET.nii.gz";
        QStringList command;

        // --- Rigid registration ---
        emit progress(0.1f, "Rigid registration");
        command << "animaPyramidalBMRegistration"
                << "-m" << m_atlasImage << "-r" << imgPath << "-o" << (prefix + "_rig.nrrd") << "-O"
                << (prefix + "_rig_tr.txt") << "--sp" << "3";
        command += m_pyramidOption;
        runCommand(command);

        qDebug() << "Rigid registration completed in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Affine registration ---
        emit progress(0.2f, "Affine registration");
        command.clear();
        command << "animaPyramidalBMRegistration"
                << "-m" << m_atlasImage << "-r" << imgPath << "-o" << (prefix + "_aff.nrrd") << "-O"
                << (prefix + "_aff_tr.txt") << "-i" << (prefix + "_rig_tr.txt") << "--sp" << "3"
                << "--ot" << "2";
        command += m_pyramidOption;
        runCommand(command);

        qDebug() << "Affine registration completed in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Base crop mask ---
        emit progress(0.3f, "Creating base crop mask");
        command.clear();
        command << "animaCreateImage"
                << "-g" << m_atlasImage << "-b" << "1"
                << "-o" << (prefix + "_baseCropMask.nrrd");
        runCommand(command);

        qDebug() << "Base crop mask created in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Transform serie ---
        emit progress(0.4f, "Generating transform series");
        command.clear();
        command << "animaTransformSerieXmlGenerator"
                << "-i" << (prefix + "_aff_tr.txt") << "-o" << (prefix + "_aff_tr.xml");
        runCommand(command);

        qDebug() << "Transform series generated in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Apply transform ---
        emit progress(0.5f, "Applying transform");
        command.clear();
        command << "animaApplyTransformSerie"
                << "-i" << (prefix + "_baseCropMask.nrrd") << "-t" << (prefix + "_aff_tr.xml")
                << "-g" << imgPath << "-o" << (prefix + "_cropMask.nrrd") << "-n" << "nearest";
        runCommand(command);

        qDebug() << "Transform applied in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Mask image ---
        emit progress(0.6f, "Masking image");
        command.clear();
        command << "animaMaskImage"
                << "-i" << imgPath << "-m" << (prefix + "_cropMask.nrrd") << "-o"
                << (prefix + "_c.nrrd");
        runCommand(command);

        qDebug() << "Image masked in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Dense registration ---
        emit progress(0.7f, "Dense registration");
        command.clear();
        command << "animaDenseSVFBMRegistration"
                << "-r" << (prefix + "_c.nrrd") << "-m" << (prefix + "_aff.nrrd") << "-o"
                << (prefix + "_nl.nrrd") << "-O" << (prefix + "_nl_tr.nrrd") << "-T" << "0" << "--tub"
                << "2";
        command += m_pyramidOption;
        runCommand(command);

        qDebug() << "Dense registration completed in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Transform serie (non-linear) ---
        emit progress(0.8f, "Generating non-linear transform");
        command.clear();
        command << "animaTransformSerieXmlGenerator"
                << "-i" << (prefix + "_aff_tr.txt") << "-i" << (prefix + "_nl_tr.nrrd") << "-o"
                << (prefix + "_nl_tr.xml");
        runCommand(command);

        qDebug() << "Non-linear transform generated in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Apply ICC mask ---
        emit progress(0.9f, "Applying ICC mask");
        command.clear();
        command << "animaApplyTransformSerie"
                << "-i" << m_iccImage << "-t" << (prefix + "_nl_tr.xml") << "-g" << imgPath << "-o"
                << (prefix + "_rough_brainMask.nrrd") << "-n" << "nearest";
        runCommand(command);

        qDebug() << "ICC mask applied in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Final mask ---
        command.clear();
        command << "animaMaskImage"
                << "-i" << imgPath << "-m" << (prefix + "_rough_brainMask.nrrd") << "-o"
                << (prefix + "_rough_masked.nrrd");
        runCommand(command);

        qDebug() << "Final mask created in" << timer.elapsed() / 1000.0 << "seconds";
        timer.restart();

        // --- Convert outputs ---
        emit progress(0.95f, "Converting outputs");
        command.clear();
        command << "animaConvertImage"
                << "-i" << (prefix + "_rough_masked.nrrd") << "-o" << maskedBrain;
        runCommand(command);

        qDebug() << "Outputs converted in" << timer.elapsed() / 1000.0 << "seconds";

        emit progress(1.0f, "Brain extraction finished");
        emit finished(maskedBrain);

        return maskedBrain;
    } catch (const std::exception &e) {
        qDebug() << "Brain extraction failed:" << e.what();
        throw std::runtime_error(std::string("Brain extraction failed: ") + e.what());
    }
}