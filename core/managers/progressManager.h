#pragma once

#include <atomic>

#include <QObject>
#include <QTimer>

class ProgressManager : public QObject {
    Q_OBJECT
  signals:
    void progressUpdated(int globalValue);
    void progressStatusChanged(const QString &status);
    void interruptionRequested();

  public:
    static ProgressManager &instance();

    void setFileName(const QString &fileName);

    void report(double stepStart, double stepWeight, int internalPercentage,
                const QString *status = nullptr);

    void requestInterruption() { 
        m_interruptionRequested = true; 
        emit interruptionRequested();
    }

    bool isInterrupted() const { return m_interruptionRequested; }

    void reset();

  private:
    QString m_baseStatus = "Initializing";
    int m_dotCount = 0;
    QTimer *m_dotTimer;
    std::atomic<bool> m_interruptionRequested{false};
    QString m_fileName;

  private:
    ProgressManager();

    void animateDots();
};