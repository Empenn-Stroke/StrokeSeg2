#include "brainextraction.h"


#include <stdexcept>

BrainExtraction::BrainExtraction(AnimaWrapper *wrapper, const QString &atlasImage, QObject *parent)
    : QObject(parent), m_wrapper(wrapper), m_atlasImage(atlasImage),
      m_iccImage(QDir(ATLAS_DIR).filePath("BrainMask.nrrd")),
      m_pyramidOption({"-p", "4", "-l", "1"}) {}

void BrainExtraction::requestCancel() {
    m_cancelRequested = true;
}

void BrainExtraction::runCommand(const QStringList &command) {
    if (m_cancelRequested) {
        throw std::runtime_error("Brain extraction cancelled by user");
    }

    m_wrapper->run(command);
}

QString BrainExtraction::run(const QString &imgPath, const QString &prefix) {
    try {
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

        // --- Affine registration ---
        emit progress(0.2f, "Affine registration");
        command.clear();
        command << "animaPyramidalBMRegistration"
                << "-m" << m_atlasImage << "-r" << imgPath << "-o" << (prefix + "_aff.nrrd") << "-O"
                << (prefix + "_aff_tr.txt") << "-i" << (prefix + "_rig_tr.txt") << "--sp" << "3"
                << "--ot" << "2";
        command += m_pyramidOption;
        runCommand(command);

        // --- Base crop mask ---
        emit progress(0.3f, "Creating base crop mask");
        command.clear();
        command << "animaCreateImage"
                << "-g" << m_atlasImage << "-b" << "1"
                << "-o" << (prefix + "_baseCropMask.nrrd");
        runCommand(command);

        // --- Transform serie ---
        emit progress(0.4f, "Generating transform series");
        command.clear();
        command << "animaTransformSerieXmlGenerator"
                << "-i" << (prefix + "_aff_tr.txt") << "-o" << (prefix + "_aff_tr.xml");
        runCommand(command);

        // --- Apply transform ---
        emit progress(0.5f, "Applying transform");
        command.clear();
        command << "animaApplyTransformSerie"
                << "-i" << (prefix + "_baseCropMask.nrrd") << "-t" << (prefix + "_aff_tr.xml")
                << "-g" << imgPath << "-o" << (prefix + "_cropMask.nrrd") << "-n" << "nearest";
        runCommand(command);

        // --- Mask image ---
        emit progress(0.6f, "Masking image");
        command.clear();
        command << "animaMaskImage"
                << "-i" << imgPath << "-m" << (prefix + "_cropMask.nrrd") << "-o"
                << (prefix + "_c.nrrd");
        runCommand(command);

        // --- Dense registration ---
        emit progress(0.7f, "Dense registration");
        command.clear();
        command << "animaDenseSVFBMRegistration"
                << "-r" << (prefix + "_c.nrrd") << "-m" << (prefix + "_aff.nrrd") << "-o"
                << (prefix + "_nl.nrrd") << "-O" << (prefix + "_nl_tr.nrrd") << "--tub" << "2";
        runCommand(command);

        // --- Transform serie (non-linear) ---
        emit progress(0.8f, "Generating non-linear transform");
        command.clear();
        command << "animaTransformSerieXmlGenerator"
                << "-i" << (prefix + "_aff_tr.txt") << "-i" << (prefix + "_nl_tr.nrrd") << "-o"
                << (prefix + "_nl_tr.xml");
        runCommand(command);

        // --- Apply ICC mask ---
        emit progress(0.9f, "Applying ICC mask");
        command.clear();
        command << "animaApplyTransformSerie"
                << "-i" << m_iccImage << "-t" << (prefix + "_nl_tr.xml") << "-g" << imgPath << "-o"
                << (prefix + "_rough_brainMask.nrrd") << "-n" << "nearest";
        runCommand(command);

        // --- Final mask ---
        command.clear();
        command << "animaMaskImage"
                << "-i" << imgPath << "-m" << (prefix + "_rough_brainMask.nrrd") << "-o"
                << (prefix + "_rough_masked.nrrd");
        runCommand(command);

        // --- Convert outputs ---
        emit progress(0.95f, "Converting outputs");
        command.clear();
        command << "animaConvertImage"
                << "-i" << (prefix + "_rough_masked.nrrd") << "-o" << maskedBrain;
        runCommand(command);

        emit progress(1.0f, "Brain extraction finished");
        emit finished(maskedBrain);

        return maskedBrain;
    } catch (const std::exception &e) {
        emit error(e.what());
        return {};
    }
}