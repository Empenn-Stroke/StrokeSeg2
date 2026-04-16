#pragma once

#include <cstdlib>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include <QtGlobal>
#include <QCoreApplication>
#include "str.h"

namespace Paths {

    inline QString roaming() {
        return QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    }

    inline QString local() {
        return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }

    inline QString programData() {
        QString path = qEnvironmentVariable("ProgramData");
        return path.isEmpty() ? "C:/ProgramData" : path;
    }

    inline QString baseDir() {
        return QDir(QFileInfo(__FILE__).absolutePath()).filePath("../../..");
    }

    inline QString animaRootPath() {
        return QDir(QCoreApplication::applicationDirPath()).filePath(ANIMA_RELATIVE_PATH);
    }

    inline QString modelDir() {
        return QDir(programData()).filePath(app_name + "/Model/");
    }

    inline QString atlasDir() {
        return QDir(programData()).filePath(app_name + "/Atlas/");
    }

    inline QString configPath() {
        return QDir(roaming()).filePath(app_name + "/config.ini");
    }

    inline QString defaultOutputDir() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath(app_name);
    }

} // namespace Paths
