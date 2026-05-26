#include "dicomConverter.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

bool DicomConverter::requiresConversion(const QString &path) {
    QStringList supportedFormats = {"nii", "nii.gz", "nrrd"};

    QFileInfo info(path);
    if (info.isDir())
        return true;

    QString suffix = info.suffix().toLower();
    QString completeSuffix = info.completeSuffix().toLower();

    if (supportedFormats.contains(suffix) || supportedFormats.contains(completeSuffix)) {
        return false;
    }

    return true;
}

QString DicomConverter::convert(const QString &inputPath, const QString &outputDir) {
    QFileInfo fileInfo(inputPath);
    QString baseName = fileInfo.completeBaseName();

    QDir().mkpath(outputDir);

    QString executable =
        QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/anima/dcm2niix.exe");

    if (!QFile::exists(executable)) {
        qCritical() << "ERREUR : dcm2niix.exe introuvable :" << executable;
        return "";
    }

    QStringList arguments;
    arguments << "-f" << baseName
              << "-z" << "y"
              << "-s" << "y"
              << "-o" << QDir::toNativeSeparators(outputDir) << QDir::toNativeSeparators(inputPath);

    qDebug() << "Conversion DICOM -> NIfTI :" << baseName;

    QProcess process;
    process.start(executable, arguments);

    if (!process.waitForFinished(60000)) {
        qCritical() << "Timeout dcm2niix.";
        return "";
    }

    QString expectedFilePath = outputDir + "/" + baseName + ".nii.gz";

    if (QFile::exists(expectedFilePath)) {
        return expectedFilePath;
    } else {
        QDir dir(outputDir);
        QStringList files = dir.entryList(QStringList() << baseName + "*.nii.gz", QDir::Files);
        if (!files.isEmpty()) {
            return dir.absoluteFilePath(files.first());
        }
    }

    qCritical() << "Échec de la conversion : le fichier NIfTI n'a pas été généré.";
    return "";
}
