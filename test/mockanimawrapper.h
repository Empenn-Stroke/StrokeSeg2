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
                    // On garde 64 pour la rapidité des tests unitaires
                    emergencyVol.data = Eigen::Tensor<float, 4, Eigen::RowMajor>(1, 64, 64, 64);
                    emergencyVol.data.setConstant(1.0f);
                    for (int x = 16; x < 48; ++x)
                        for (int y = 16; y < 48; ++y)
                            for (int z = 16; z < 48; ++z)
                                emergencyVol.data(0, x, y, z) = 100.0f;

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