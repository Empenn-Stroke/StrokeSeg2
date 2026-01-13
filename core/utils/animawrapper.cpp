#include "animawrapper.h"
#include "path.h"
#include <iostream>

AnimaWrapper::AnimaWrapper(QObject *parent)
    : QObject(parent),
      process(this),
      m_stdout(),
      m_stderr()
{
    // Utiliser des canaux séparés pour capturer stdout et stderr distinctement.
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

    // Si un précédent processus est encore en cours, tenter de l'arrêter.
    if (process.state() != QProcess::NotRunning) {
        process.kill();
        process.waitForFinished(3000);
    }

    // Configurer et démarrer le processus

    std::cout << "Starting process: " << (anima_root_path + program).toStdString()
              << " "
              << arguments.join(' ').toStdString() << std::endl;

    process.setProgram(anima_root_path + program);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start();

    // Attendre le démarrage (5s).
    if (!process.waitForStarted(5000)) {
        m_stderr = process.errorString();
        if (m_stderr.isEmpty())
            m_stderr = QStringLiteral("Failed to start process");
        return -1;
    }

    // Attendre la fin (blocage indéfini).
    process.waitForFinished(-1);

    // Lire les sorties
    const QByteArray out = process.readAllStandardOutput();
    const QByteArray err = process.readAllStandardError();
    m_stdout = QString::fromUtf8(out);
    m_stderr = QString::fromUtf8(err);

    // Vérifier si le processus s'est terminé anormalement
    if (process.exitStatus() == QProcess::CrashExit) {
        // Retourner -1 pour signaler l'échec par crash
        return -1;
    }

    return process.exitCode();
}