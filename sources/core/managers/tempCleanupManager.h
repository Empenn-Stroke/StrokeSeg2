// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>

namespace TempCleanupManager {
    /**
     * @brief Cleans up old temporary directories.
     *
     * For each directory in outputDirs, this function checks whether it contains a "_tmp"
     * subdirectory that is older than the specified maximum age in days. If such a subdirectory
     * is found, it is removed. This function is intended to be called at application startup
     * as a safety net for _tmp directories left behind by a crash or forced termination.
     * Normal exits are already cleaned up by TempDirGuard in PipelineWorker.
     *
     * @param outputDirs A list of directories to check for old "_tmp" subdirectories.
     * @param maxAgeDays The maximum age in days for a "_tmp" subdirectory to be considered
     *                   old and eligible for removal. Default is 7 days.
     */
    void cleanupOldTempDirs(const QStringList &outputDirs, int maxAgeDays = 7);
} // namespace TempCleanupManager
