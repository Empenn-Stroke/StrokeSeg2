// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QMessageLogContext>
#include <QString>
#include <QtGlobal>

/**
 * @brief A namespace for managing application logging.
 *
 * This namespace provides functions for handling log messages, retrieving the log file path,
 * and cleaning up old log files.
 */
namespace LogManager {
    /**
     * @brief Custom message handler for logging messages.
     *
     * This function is called by Qt's logging framework to handle log messages. It processes
     * the message based on its type and logs it to the appropriate destination.
     *
     * @param type The type of the log message (e.g., QtDebugMsg, QtWarningMsg).
     * @param context The context information for the log message.
     * @param msg The log message string.
     */
    void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    /**
     * @brief Get the path to the current log file.
     *
     * @return QString The path to the current log file.
     */
    QString getLogFilePath();

    /**
     * @brief Cleanup old log files.
     *
     * This function removes log files that are older than the specified number of days.
     *
     * @param daysToKeep The number of days to keep log files. Default is 30 days.
     */
    void cleanupOldLogs(int daysToKeep = 30);
} // namespace LogManager
