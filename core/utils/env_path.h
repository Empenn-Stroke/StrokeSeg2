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

    inline QDir roaming() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }

    inline QDir local() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    }

    inline QDir programData() {
        QString path = qEnvironmentVariable("ProgramData");
        return path.isEmpty() ? QDir("C:/ProgramData") : QDir(path);
    }

    inline QDir baseDir() {
        return QDir(QFileInfo(__FILE__).absolutePath()).filePath("../../..");
    }

    inline QDir animaRootPath() {
        return QDir(QCoreApplication::applicationDirPath()).filePath(ANIMA_RELATIVE_PATH);
    }

    inline QDir modelDir() {
        return QDir(programData()).filePath(app_name + "/Model/");
    }

    inline QDir atlasDir() {
        return QDir(programData()).filePath(app_name + "/Atlas/");
    }

    inline QDir configPath() {
        return QDir(roaming()).filePath(app_name + "/config.ini");
    }

    inline QDir defaultOutputDir() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).filePath(app_name);
    }

} // namespace Paths
