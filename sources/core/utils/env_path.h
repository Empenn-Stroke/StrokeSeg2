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
    /**
     * @brief Get the roaming directory path.
     * @return QDir object representing the roaming directory.
     */
    inline QDir roaming() 
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    }

    /**
     * @brief Get the local directory path.
     * @return QDir object representing the local directory.
     */
    inline QDir local() 
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
    }

    /**
     * @brief Get the ProgramData directory path.
     * @return QDir object representing the ProgramData directory.
     */
    inline QDir programData() 
    {
        QString path = qEnvironmentVariable("ProgramData");
        return path.isEmpty() ? QDir("C:/ProgramData") : QDir(path);
    }

    /**
     * @brief Get the models directory path.
     * @return QDir object representing the models directory.
     */
    inline QDir modelDir() 
    {
        return QDir(programData()).filePath(appName + "/Models/");
    }

    /**
     * @brief Get the atlas directory path.
     * @return QDir object representing the atlas directory.
     */
    inline QDir atlasDir() 
    {
        return QDir(programData()).filePath(appName + "/Atlas/");
    }

    /**
     * @brief Get the configuration path.
     * @return QDir object representing the configuration path.
     */
    inline QDir configPath() 
    {
        return QDir(roaming()).filePath(appName + "/config.ini");
    }

    /**
     * @brief Get the default output directory path.
     * @return QDir object representing the default output directory.
     */
    inline QDir defaultOutputDir() 
    {
        return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
            .filePath(appName);
    }

#elif defined(Q_OS_MAC)
    /**
     * @brief Get the roaming directory path.
     * @return QDir object representing the roaming directory.
     */
    inline QDir roaming() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        return result;
    }

    /**
     * @brief Get the bundle resources directory path.
     * @return QDir object representing the bundle resources directory.
     */
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

    /**
     * @brief Get the models directory path.
     * @return QDir object representing the models directory.
     */
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

    /**
     * @brief Get the atlas directory path.
     * @return QDir object representing the atlas directory.
     */
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

    /**
     * @brief Get the configuration path.
     * @return QDir object representing the configuration path.
     */
    inline QDir configPath() 
    {
        QDir result = QDir(roaming()).filePath("config.ini");
        return result;
    }

    /**
     * @brief Get the default output directory path.
     * @return QDir object representing the default output directory.
     */
    inline QDir defaultOutputDir() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                      .filePath(appName);
        return result;
    }

#elif defined(Q_OS_LINUX)
    /**
     * @brief Get the roaming directory path.
     * @return QDir object representing the roaming directory.
     */
    inline QDir roaming() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
        return result;
    }

    /**
     * @brief Get the local directory path.
     * @return QDir object representing the local directory.
     */
    inline QDir local() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation));
        return result;
    }

    /**
     * @brief Get the models directory path.
     * @return QDir object representing the models directory.
     */
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

    /**
     * @brief Get the atlas directory path.
     * @return QDir object representing the atlas directory.
     */
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

    /**
     * @brief Get the configuration path.
     * @return QDir object representing the configuration path.
     */
    inline QDir configPath() 
    {
        QDir result = QDir(roaming()).filePath("config.ini");
        return result;
    }

    /**
     * @brief Get the default output directory path.
     * @return QDir object representing the default output directory.
     */
    inline QDir defaultOutputDir() 
    {
        QDir result = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                      .filePath(appName);
        return result;
    }

#endif

    /**
     * @brief Get the base directory path.
     * @return QDir object representing the base directory.
     */
    inline QDir baseDir() 
    {
        return QDir(QCoreApplication::applicationDirPath());
    }

    /**
     * @brief Get the Anima root directory path.
     * @return QDir object representing the Anima root directory.
     */
    inline QDir animaRootPath() 
    {
        return QDir(baseDir().filePath(ANIMA_RELATIVE_PATH));
    }

} // namespace Paths
