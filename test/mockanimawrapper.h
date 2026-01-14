#pragma once

#include <utils/animawrapper.h>

class MockAnimaWrapper : public AnimaWrapper {
  public:
    QStringList lastCommand;
    int callCount = 0;
    bool shouldFail = false;

    explicit MockAnimaWrapper(QObject *parent = nullptr) : AnimaWrapper(parent) {}

    int run(const QStringList &args) override {
        callCount++;
        int outIdx = args.indexOf("-o");
        if (outIdx != -1 && outIdx + 1 < args.size()) {
            QString outputPath = args.at(outIdx + 1);

            // SÉCURITÉ : Créer le dossier parent si nécessaire
            QDir().mkpath(QFileInfo(outputPath).absolutePath());

            int inIdx = args.indexOf("-i");
            if (inIdx != -1 && inIdx + 1 < args.size()) {
                QFile::remove(outputPath);
                if (!QFile::copy(args.at(inIdx + 1), outputPath)) {
                    // Si la copie échoue (ex: chemin invalide), on crée au moins un fichier vide
                    // pour que le loader NIfTI ne lance pas d'exception fatale
                    QFile file(outputPath);
                    file.open(QIODevice::WriteOnly);
                    file.close();
                }
            }
        }
        return 0;
    }
};