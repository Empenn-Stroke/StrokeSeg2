#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>

#ifndef CORE_UTILS_ANIMANWRAPPER_H
#define CORE_UTILS_ANIMANWRAPPER_H

class AnimaWrapper : public QObject {
    Q_OBJECT
    public:
    explicit AnimaWrapper(QObject *parent = nullptr);

    // Runs command synchronously. Returns exit code.
    int run(const QStringList &args);

    // Optional: get last stderr/stdout
    QString lastStdout() const { return m_stdout; }
    QString lastStderr() const { return m_stderr; }

    private:
    QProcess process;
    QString m_stdout;
    QString m_stderr;
};


#endif
