#pragma once

#include <utils/animawrapper.h>
#include "test_utils.h"

class MockAnimaWrapper : public AnimaWrapper {
  public:
    QStringList lastCommand;
    int callCount = 0;
    bool shouldFail = false;

    explicit MockAnimaWrapper(QObject *parent = nullptr) : AnimaWrapper(parent) {}

    int run(const QStringList &args) override {
        this->callCount++;
        this->lastCommand = args;

        if (shouldFail)
            return 1;

        // 1. On cherche TOUS les fichiers de sortie potentiels dans la commande
        for (int i = 0; i < args.size(); ++i) {
            QString currentArg = args[i];

            // Si l'argument est un flag de sortie (-o ou -O)
            if ((currentArg == "-o" || currentArg == "-O") && i + 1 < args.size()) {
                QString outputPath = args[i + 1];
                QDir().mkpath(QFileInfo(outputPath).absolutePath());

                // CAS 1 : C'est l'image de sortie principale
                if (outputPath.endsWith(".nii") || outputPath.endsWith(".nii.gz")) {
                    NiftiVolume emergencyVol;

                    // CORRECTION : Passage en ColMajor et ordre (X, Y, Z, C)
                    // On garde 64x64x64x1
                    emergencyVol.data = Eigen::Tensor<float, 4, Eigen::ColMajor>(64, 64, 64, 1);
                    emergencyVol.data.setConstant(1.0f);

                    // Remplissage d'un cube central pour simuler un "objet"
                    for (int z = 16; z < 48; ++z)
                        for (int y = 16; y < 48; ++y)
                            for (int x = 16; x < 48; ++x)
                                emergencyVol.data(x, y, z, 0) = 100.0f;

                    emergencyVol.spacing = Eigen::Vector3f(1.0f, 1.0f, 1.0f);

                    if (!NiftiVolume::saveNifti(outputPath, emergencyVol))
                        return 1;
                }
                // CAS 2 : C'est le fichier de transformation ou un log (.txt, .nrrd, etc.)
                else {
                    QFile file(outputPath);
                    if (file.open(QIODevice::WriteOnly)) {
                        file.write("Dummy transformation matrix or non-nifti data");
                        file.close();
                    }
                }
            }
        }
        return 0;
    }
};