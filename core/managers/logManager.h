#pragma once
#include <QMessageLogContext>
#include <QString>
#include <QtGlobal>

namespace LogManager {
    void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    QString getLogFilePath();
    void cleanupOldLogs(int daysToKeep = 30);
} // namespace LogManager