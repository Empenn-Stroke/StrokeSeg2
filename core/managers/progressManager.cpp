#include <managers/progressManager.h>

ProgressManager &ProgressManager::instance() {
    static ProgressManager inst;
    return inst;
}

void ProgressManager::report(double stepStart, double stepWeight, int internalPercentage,
                                    const QString *status) {
    int global = static_cast<int>(stepStart + (stepWeight * internalPercentage / 100.0));
    emit progressUpdated(global);
    if (status && !status->isEmpty()) {
        m_baseStatus = *status;
        emit progressStatusChanged(m_baseStatus + QString(".").repeated(m_dotCount));
    }

    if (isInterrupted()) {
        throw std::runtime_error("Cancelled");
    }
}

void ProgressManager::reset() {
    m_baseStatus = "Initializing";
    m_dotCount = 0;
    emit progressUpdated(0);
    emit progressStatusChanged(m_baseStatus);

    m_interruptionRequested = false;
}

ProgressManager::ProgressManager() {
    m_dotTimer = new QTimer(this);
    connect(m_dotTimer, &QTimer::timeout, this, &ProgressManager::animateDots);
    m_dotTimer->start(500);
}

void ProgressManager::animateDots() {
    m_dotCount = (m_dotCount + 1) % 4;

    QString visibleDots = QString(".").repeated(m_dotCount);
    QString invisibleDots =
        QString("<span style='color:transparent;'>.</span>").repeated(3 - m_dotCount);

    emit progressStatusChanged(m_baseStatus + visibleDots + invisibleDots);
}
