// SPDX-License-Identifier: AGPL-3.0-or-later

#include <managers/progressManager.h>

const int MAX_FILENAME_LENGTH = 30;

ProgressManager &ProgressManager::instance() 
{
    static ProgressManager inst;
    return inst;
}

void ProgressManager::setFileName(const QString &fileName) 
{
    m_fileName = fileName;
}

void ProgressManager::report(double stepStart, double stepWeight, int internalPercentage, const QString *status) 
{
    int global = static_cast<int>(stepStart + (stepWeight * internalPercentage / 100.0));
    emit progressUpdated(global);
    if (status && !status->isEmpty()) 
    {
        QString displayFileName = m_fileName;
        if (displayFileName.length() > MAX_FILENAME_LENGTH)
        {
            displayFileName = m_fileName.left(MAX_FILENAME_LENGTH / 2) + "..." + m_fileName.right(MAX_FILENAME_LENGTH / 2 - 3);
        }

        m_baseStatus = displayFileName + " : " + *status;

        emit progressStatusChanged(m_baseStatus + QString(".").repeated(m_dotCount));
    }

    if (isInterrupted())
    {
        throw std::runtime_error("Cancelled");
    }
}

void ProgressManager::reset() 
{
    m_baseStatus = "Initializing";
    m_dotCount = 0;
    emit progressUpdated(0);
    emit progressStatusChanged(m_baseStatus);

    m_interruptionRequested = false;
}

ProgressManager::ProgressManager() 
{
    m_dotTimer = new QTimer(this);
    connect(m_dotTimer, &QTimer::timeout, this, &ProgressManager::animateDots);
    m_dotTimer->start(500);
}

void ProgressManager::animateDots()
{
    m_dotCount = (m_dotCount + 1) % 4;

    QString visibleDots = QString(".").repeated(m_dotCount);
    QString invisibleDots = QString("<span style='color:transparent;'>.</span>").repeated(3 - m_dotCount);

    emit progressStatusChanged(m_baseStatus + visibleDots + invisibleDots);
}
