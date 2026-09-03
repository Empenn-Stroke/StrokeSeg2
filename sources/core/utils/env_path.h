// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#ifdef APP_NAME
    const QString appName = APP_NAME;
#endif

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QString>
#include <QtGlobal>
#include <cstdlib>

namespace Paths {

#if defined(Q_OS_WIN)
    inline QDir roaming() 
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }

    inline QDir local() 
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    }

    inline QDir programData() 
    {
        QString path = qEnvironmentVariable("ProgramData");
        return path.isEmpty() ? QDir("C:/ProgramData") : QDir(path);
    }

    inline QDir modelDir() 
    {
        return QDir(programData()).filePath(appName + "/Models/");
    }

    inline QDir atlasDir() 
    {
        return QDir(programData()).filePath(appName + "/Atlas/");
    }

    inline QDir configPath() 
    {
        return QDir(roaming()).filePath(appName + "/config.ini");
    }

    inline QDir defaultOutputDir() 
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .filePath(appName);
    }

#elif defined(Q_OS_MAC)
    inline QDir roaming() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        return result;
    }

    inline QDir bundleResourcesDir() 
    {
        QDir appDir(QCoreApplication::applicationDirPath());
        
        if (appDir.dirName() == "MacOS") 
        {
            appDir.cdUp();
            appDir.cd("Resources");
        }
        
        return appDir;
    }

    inline QDir modelDir() 
    {
        QDir result;
        QString prodPath = bundleResourcesDir().filePath("Models");
        QString devPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../AppData/Models");

        if (QFileInfo(prodPath).isDir()) 
        {
            result = QDir(prodPath);
        } 
        else if (QFileInfo(devPath).isDir()) 
        {
            result = QDir(devPath);
        } 
        else 
        {
            result = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                     .filePath(appName + "/Models/");
        }

        return result;
    }

    inline QDir atlasDir() 
    {
        QDir result;
        QString prodPath = bundleResourcesDir().filePath("Atlas");
        QString devPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../AppData/Atlas");

        if (QFileInfo(prodPath).isDir()) 
        {
            result = QDir(prodPath);
        } 
        else if (QFileInfo(devPath).isDir()) 
        {
            result = QDir(devPath);
        } 
        else 
        {
            result = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation))
                     .filePath(appName + "/Atlas/");
        }

        return result;
    }

    inline QDir configPath() 
    {
        QDir result = QDir(roaming()).filePath("config.ini");
        return result;
    }

    inline QDir defaultOutputDir() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                      .filePath(appName);
        return result;
    }

#elif defined(Q_OS_LINUX)
    inline QDir roaming() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        return result;
    }

    inline QDir local() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        return result;
    }

    inline QDir modelDir() 
    {
        QDir result;
        QString devPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../AppData/Models");
        QString prodPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../Models");

        qDebug() << "devPath: " << devPath;
        qDebug() << "prodPath: " << prodPath;

        if (QFileInfo(devPath).isDir()) 
        {
            result = QDir(devPath);
        } 
        else if (QFileInfo(prodPath).isDir()) 
        {
            result = QDir(prodPath);
        } 
        else 
        {
            result = QDir(local()).filePath("Models/");
        }

        return result;
    }

    inline QDir atlasDir() 
    {
        QDir result;
        QString devPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../../AppData/Atlas");
        QString prodPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/../Atlas");

        if (QFileInfo(devPath).isDir()) 
        {
            result = QDir(devPath);
        } 
        else if (QFileInfo(prodPath).isDir()) 
        {
            result = QDir(prodPath);
        } 
        else 
        {
            result = QDir(local()).filePath("Atlas/");
        }

        return result;
    }

    inline QDir configPath() 
    {
        QDir result = QDir(roaming()).filePath("config.ini");
        return result;
    }

    inline QDir defaultOutputDir() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                      .filePath(appName);
        return result;
    }

#endif

    inline QDir baseDir() 
    {
        return QDir(QCoreApplication::applicationDirPath());
    }

    inline QDir animaRootPath() 
    {
        return QDir(baseDir().filePath(ANIMA_RELATIVE_PATH));
    }

} // namespace Paths
