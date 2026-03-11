#include "animawrapper.h"
#include "path.h"
#include <iostream>

AnimaWrapper::AnimaWrapper(QObject *parent)
    : QObject(parent),
      process(this),
      m_stdout(),
      m_stderr()
{
    // Use separate channels to get stdout and stderr
    process.setProcessChannelMode(QProcess::SeparateChannels);
}

int AnimaWrapper::run(const QStringList &args) {
    m_stdout.clear();
    m_stderr.clear();

    if (args.isEmpty()) {
        m_stderr = QStringLiteral("No program specified");
        return -1;
    }

    const QString program = args.first();
    const QStringList arguments = args.mid(1);

    // If a precedent process is still running, try to stop it.
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(3000);
    }

    // Configure and start process.
    QString program_path = QDir(anima_root_path).filePath(program);

    std::cout << "Starting process: " << program_path.toStdString()
              << " "
              << arguments.join(' ').toStdString() << std::endl;

    process.setProgram(program_path);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();

    // Wait for start.
    if (!process.waitForStarted(5000)) {
        m_stderr = process.errorString();
        if (m_stderr.isEmpty())
            m_stderr = QStringLiteral("Failed to start process");
        return -1;
    }

    // Wait for end.
    process.waitForFinished(-1);

    // Read outputs.
    const QByteArray out = process.readAllStandardOutput();
    const QByteArray err = process.readAllStandardError();
    m_stdout = QString::fromUtf8(out);
    m_stderr = QString::fromUtf8(err);

    if (process.exitCode() != 0) {
        qDebug() << "--- ANIMA CRASH LOG ---";
        qDebug() << "STDOUT:" << m_stdout;
        qDebug() << "STDERR:" << m_stderr;
    }

    // Check if process abnormally stopped.
    if (process.exitStatus() == QProcess::CrashExit) {
        // Return -1 to notify the crash.
        return -1;
    }

    return process.exitCode();
}