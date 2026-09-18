// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QtCore/QtGlobal>

#ifndef CORE_UTILS_ANIMAWRAPPER_H
#define CORE_UTILS_ANIMAWRAPPER_H

/**
 * @brief A class to wrap and manage the execution of Anima commands.
 *
 * This class provides functionality to run Anima commands synchronously and
 * manage the output and error streams. It also emits signals for logging and error handling.
 *
 * @details This class uses QProcess to execute external commands and capture their output.
 * It is designed to be used in a Qt application environment and provides a clean interface
 * for running Anima commands and handling their results.
 */
class AnimaWrapper : public QObject
{
    Q_OBJECT
  public:
    /**
     * @brief Constructor for AnimaWrapper.
     *
     * @param parent The parent QObject of this instance.
     */
    explicit AnimaWrapper(QObject *parent = nullptr);

    /**
     * @brief Runs an Anima command with the specified arguments.
     *
     * @param args A list of arguments to pass to the Anima command.
     * @return The exit code of the command.
     */
    virtual int run(const QStringList &args);

    /**
     * @brief The current process being managed by the wrapper.
     */
    QProcess *m_currentProcess = nullptr;

    /**
     * @brief Aborts the currently running process.
     */
    void abort();

    /**
     * @brief Retrieves the last captured standard output from the process.
     *
     * @return A QString containing the last standard output.
     */
    QString lastStdout() const { return m_stdout; }

    /**
     * @brief Retrieves the last captured standard error from the process.
     *
     * @return A QString containing the last standard error.
     */
    QString lastStderr() const { return m_stderr; }

  protected:
    /**
     * @brief The QProcess object used to execute external commands.
     */
    QProcess process;

    /**
     * @brief Stores the last captured standard output from the process.
     */
    QString m_stdout;

    /**
     * @brief Stores the last captured standard error from the process.
     */
    QString m_stderr;

  signals:
    /**
     * @brief Signal emitted when new log data is available.
     *
     * @param message A QString containing the log message.
     */
    void logAvailable(const QString &message);

    /**
     * @brief Signal emitted when an error occurs during process execution.
     *
     * @param error A QString containing the error message.
     */
    void errorOccurred(const QString &error);
};

#endif // CORE_UTILS_ANIMAWRAPPER_H
