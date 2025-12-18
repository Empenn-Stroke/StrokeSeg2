#include "brainextraction.h"

#include "animawrapper.h"
#include "mainwindow.h"
#include "path.h"

#include <Qdir>
#include <stdexcept>


//Constructor
BrainExtraction::BrainExtraction(AnimaWrapper *wrapper, const QString &atlasImage, MainWindow *gui)
    : m_wrapper(wrapper), 
      m_atlasImage(atlasImage),
      m_iccImage(Qdir(ATLAS_DIR).filePath("BrainMask.nrrd")),
      m_pyramidOption({"-p", "4", "-l", "1"}),
      m_gui(gui)    
{

}

void BrainExtraction::runCommand(const QStringList &command) {
    m_wrapper->run(command);

    if (m_gui != nullptr && m_gui->checkStop()) {
        throw std::runtime_error("Action was cancelled by the user.");
    }
}

QString BrainExtraction::run(const QString &imgPath, const QString &prefix) {

    QString brainMask = prefix + "_brainMask.nii.gz";
    QString maskedBrain = prefix + "_BET.nii.gz";
    QStringList command;

    //Rigid registration
    command << "animaPyramidalBMRegistration"
            << "-m" << m_atlasImage << "-r" << imgPath 
            << "-o" << (prefix + "_rig.nrrd") 
            << "-O" << (prefix + "_rig_tr.txt") 
            << "--sp" << "3";
    command += m_pyramidOption;
    runCommand(command);
    
    //Affine registration
    command.clear();
    command << "animaPyramidalBMRegistration"
            << "-m" << m_atlasImage << "-r" << imgPath 
            << "-o" << (prefix + "_aff.nrrd") 
            << "-O" << (prefix + "_aff_tr.txt") 
            << "-i" << (prefix + "_rig_tr.txt") 
            << "--sp" << "3" << "--ot" << "2";
    command += m_pyramidOption;
    runCommand(command);

    // Create Base crop mask
    command.clear();
    command << "animaCreateImage"
            << "-g" << m_atlatImage << "-b" << "1" 
            << "-o" << (prefix + "_baseCropMask.nrrd");
    runCommand(command);

    // Generate anima transform serie
    command.clear();
    command << "animaTransformSerieXmlGenerator"
            << "-i" << (prefix + "_aff_tr.txt")
            << "-o" << (prefix + "_aff_tr.xml");
    runCommand(command);

    // Apply anima transform serie 
    command.clear();
    command << "animeaApplyTransofrmSerie"
            << "-i" << (prefix + "_baseCropMask.nrrd")
            << "-t" << (prefix + "_aff_tr.xml")
            << "-g" << imgPath 
            << "-o" << (prefix + "_cropMask.nrrd") 
            << "-n" << "nearest";
    runCommand(command);

    // Anima mask image
    command.clear();
    command << "animaMaskImage"
            << "-i" << imgPath
            << "-m" << (prefix + "_cropMask.nrrd")
            << "-o" (prefix + "_c.nrrd");
    runCommand(command);

    //Dense registration
    command.clear();
    command << "animaDenseSVFBMRegistration"
            << "-r" << (prefix + "_c.nrrd")
            << "-m" << (prefix + "_aff.nrrd")
            << "-o" << (prefix + "_nl.nrrd")
            << "-O" << (prefix + "_nl_tr.nrrd")
            << "--tub" << "2";
    runCommand(command);

    //Generate anima transform serie 
    command.clear();
    command << "animaTransformSerieXmlGenerator"
            << "-i" << (prefix + "_aff_tr.txt")
            << "-i" << (prefix + "_nl_tr.nrrd")
            << "-o" << (prefix + "_nl_tr.xml");
    runCommand(command);

    // Apply transform serie
    command.clear();
    command << "animaApplyTransformSerie"
            << "-i" << m_iccImage   
            << "-t" << (prefix + "_nl_tr.xml")
            << "-g" << imgPath 
            << "-o" (prefix + "_rough_brainMask.nrrd") 
            << "-n" << "nearest";
    runCommand(command);

    // anima rough masked image
    command.clear();
    command << "animaMaskImage" << "-i" << imgPath
            << "-m" << (prefix + "_rough_brainMask.nrrd") 
            << "-o" << (prefix + "_rough_masked.nrrd");
    runCommand(command);

    const QString brainImageRoughMasked = prefix + "_rough_masked.nrrd";

    // Convert image 
    command.clear();
    command << "animaConvertImage" << "-i" << brainImageRoughMasked 
            << "-o" << (prefix + "_masked.nrrd");
    runCommand(command);

    // Convert Image
    command.clear();
    command << "animaConvertImage"
            << "-i" << (prefix + "_rough_brainMask.nrrd")
            << "-o" << (prefix + "_brainMask.nrrd");
    runCommand(command);

    // Convert Image
    command.clear();
    command << "animaConvertImage"
            << "-i" << (prefix + "_masked.nrrd")
            << "-o" << maskedBrain;
    runCommand(command);

    // Convert Image
    command.clear();
    command << "animaConvertImage"
            << "-i" << (prefix + "_brainMaisk.nrrd")
            << "-o" << brainMask;
    m_wrapper->run(command);

    return maskedBrain;
}
