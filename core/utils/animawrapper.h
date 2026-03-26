#pragma once
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QtCore/QtGlobal>

#ifndef CORE_UTILS_ANIMANWRAPPER_H
#define CORE_UTILS_ANIMANWRAPPER_H

class AnimaWrapper : public QObject {
    Q_OBJECT
    public:
    explicit AnimaWrapper(QObject *parent = nullptr);

    // Runs command synchronously. Returns exit code.
    virtual int run(const QStringList &args);

    // Optional: get last stderr/stdout
    QString lastStdout() const { return m_stdout; }
    QString lastStderr() const { return m_stderr; }

    protected:
    QProcess process;
    QString m_stdout;
    QString m_stderr;

    signals:
        void logAvailable(const QString &message);
        void errorOccurred(const QString &error);
};


#endif
