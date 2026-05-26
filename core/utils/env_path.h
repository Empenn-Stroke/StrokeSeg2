#pragma once

#include "str.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>
#include <QtGlobal>
#include <cstdlib>

namespace Paths {

#if defined(Q_OS_WIN)
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

    inline QDir modelDir() {
        return QDir(programData()).filePath(app_name + "/Models/");
    }

    inline QDir atlasDir() {
        return QDir(programData()).filePath(app_name + "/Atlas/");
    }

    inline QDir configPath() {
        return QDir(roaming()).filePath(app_name + "/config.ini");
    }

    inline QDir defaultOutputDir() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .filePath(app_name);
    }

#elif defined(Q_OS_MAC)
    inline QDir roaming() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }

    inline QDir sharedData() {
        QStringList paths = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
        return paths.size() > 1 ? QDir(paths.at(1)) : QDir("/Library/Application Support");
    }

    inline QDir modelDir() {
        return QDir(sharedData()).filePath(app_name + "/Models/");
    }

    inline QDir atlasDir() {
        return QDir(sharedData()).filePath(app_name + "/Atlas/");
    }

    inline QDir configPath() {
        return QDir(roaming()).filePath("config.ini");
    }

    inline QDir defaultOutputDir() {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .filePath(app_name);
    }

#elif defined(Q_OS_LINUX)

#endif
    inline QDir baseDir() {
        return QDir(QCoreApplication::applicationDirPath());
    }

    inline QDir animaRootPath() {
        return QDir(baseDir().filePath(ANIMA_RELATIVE_PATH));
    }

} // namespace Paths