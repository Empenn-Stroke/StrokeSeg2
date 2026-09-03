// SPDX-License-Identifier: AGPL-3.0-or-later

#include "logManager.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <cstdio>

namespace LogManager 
{
    static QString currentLogFullFilepath;

    QString getLogFilePath() 
    {
        if (currentLogFullFilepath.isEmpty()) 
        {
            QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir().mkpath(path);

            QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
            QString fileName = QString("strokeseg2_%1.log").arg(timestamp);

            currentLogFullFilepath = path + "/" + fileName;
        }
        return currentLogFullFilepath;
    }

    void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) 
    {
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

        QString level;
        switch (type) 
        {
        case QtDebugMsg:
            level = "DEBUG";
            break;
        case QtWarningMsg:
            level = "WARN ";
            break;
        case QtCriticalMsg:
            level = "CRIT ";
            break;
        case QtFatalMsg:
            level = "FATAL";
            break;
        default:
            level = "INFO ";
            break;
        }

        QString txt = QString("[%1] [%2] %3").arg(timestamp, level, msg);

        QFile outFile(getLogFilePath());
        if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        {
            QTextStream ts(&outFile);
            ts << txt << Qt::endl;
        }

        fprintf(stderr, "%s\n", txt.toLocal8Bit().constData());
        fflush(stderr);
    }

    void cleanupOldLogs(int daysToKeep) 
    {
        QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(path);

        QStringList filter;
        filter << "*.log";

        QFileInfoList fileList = dir.entryInfoList(filter, QDir::Files);
        QDateTime limitDate = QDateTime::currentDateTime().addDays(-daysToKeep);

        int deletedCount = 0;
        for (const QFileInfo &fileInfo : fileList) 
        {
            if (fileInfo.lastModified() < limitDate) 
            {
                if (QFile::remove(fileInfo.absoluteFilePath())) 
                {
                    deletedCount++;
                }
            }
        }

        if (deletedCount > 0) 
        {
            qDebug() << "[Cleanup] Removed" << deletedCount << "old log files.";
        }
    }
} // namespace LogManager