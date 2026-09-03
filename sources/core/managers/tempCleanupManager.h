// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>

namespace TempCleanupManager {
    /**
     * @brief For each directory in outputDirs, checks whether it contains a "_tmp"
     * subdirectory older than maxAgeDays and removes it if so. Intended to be called
     * at application startup as a safety net for _tmp directories left behind by a
     * crash or forced termination (normal exits are already cleaned up by TempDirGuard
     * in PipelineWorker).
     */
    void cleanupOldTempDirs(const QStringList &outputDirs, int maxAgeDays = 7);
} // namespace TempCleanupManager
