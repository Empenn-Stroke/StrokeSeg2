// SPDX-License-Identifier: AGPL-3.0-or-later

#include "tempCleanupManager.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>

namespace TempCleanupManager
{
    void cleanupOldTempDirs(const QStringList &outputDirs, int maxAgeDays)
    {
        for (const QString &outputDir : outputDirs)
        {
            QString tmpPath = outputDir + "/_tmp";
            QFileInfo tmpInfo(tmpPath);

            if (tmpInfo.exists() && tmpInfo.isDir())
            {
                qint64 ageDays = tmpInfo.lastModified().daysTo(QDateTime::currentDateTime());

                if (ageDays >= maxAgeDays)
                {
                    QDir tmpDir(tmpPath);
                    bool removed = tmpDir.removeRecursively();

                    if (removed)
                    {
                        qDebug() << "[TempCleanup] Removed stale temp directory:" << tmpPath;
                    }
                    else
                    {
                        qWarning() << "[TempCleanup] Failed to remove:" << tmpPath;
                    }
                }
            }
        }
    }

} // namespace TempCleanupManager
