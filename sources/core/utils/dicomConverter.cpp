// SPDX-License-Identifier: AGPL-3.0-or-later

#include "dicomConverter.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>

bool DicomConverter::requiresConversion(const QString &path) 
{
    bool result = false;

    QStringList supportedFormats = {"nii", "nii.gz", "nrrd"};

    QFileInfo info(path);
    if (info.isDir()) 
    {
        result = true;
    }
    else
    {
        QString suffix = info.suffix().toLower();
        QString completeSuffix = info.completeSuffix().toLower();

        if (supportedFormats.contains(suffix) || supportedFormats.contains(completeSuffix)) {
            result = false;
        }
        else 
        {
            result = true;
        }
    }
    
    return result;
}

QString DicomConverter::convert(const QString &inputPath, const QString &outputDir) 
{
    QFileInfo fileInfo(inputPath);
    QString baseName = fileInfo.completeBaseName();

    QDir().mkpath(outputDir);

    QString executable;

    #if defined(Q_OS_MAC) || defined(Q_OS_LINUX)
        executable = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/dcm2niix/dcm2niix");
    #elif defined(Q_OS_WINDOWS)
        executable = QDir::toNativeSeparators(QCoreApplication::applicationDirPath() + "/dcm2niix/dcm2niix.exe");
    #endif

    if (!QFile::exists(executable)) 
    {
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

    if (!process.waitForFinished(60000)) 
    {
        qCritical() << "Timeout dcm2niix.";
        return "";
    }

    QString expectedFilePath = outputDir + "/" + baseName + ".nii.gz";

    if (QFile::exists(expectedFilePath)) 
    {
        return expectedFilePath;
    } 
    else 
    {
        QDir dir(outputDir);
        QStringList files = dir.entryList(QStringList() << baseName + "*.nii.gz", QDir::Files);
        if (!files.isEmpty()) 
        {
            return dir.absoluteFilePath(files.first());
        }
    }

    qCritical() << "Échec de la conversion : le fichier NIfTI n'a pas été généré.";
    return "";
}
