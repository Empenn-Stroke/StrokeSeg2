// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <atomic>
#include <QObject>
#include <QTimer>

/**
 * @brief A class for managing progress updates and interruptions.
 *
 * This class provides a singleton instance for managing progress updates, emitting signals
 * to indicate progress, status changes, and interruptions. It also handles animating a dot
 * indicator for ongoing operations.
 */
class ProgressManager : public QObject
{
    Q_OBJECT
  signals:
    /**
     * @brief Signal emitted when the progress value is updated.
     *
     * @param globalValue The updated progress value as an integer.
     */
    void progressUpdated(int globalValue);

    /**
     * @brief Signal emitted when the progress status changes.
     *
     * @param status The new status message as a QString.
     */
    void progressStatusChanged(const QString &status);

    /**
     * @brief Signal emitted when an interruption is requested.
     */
    void interruptionRequested();

  public:
    /**
     * @brief Get the singleton instance of ProgressManager.
     *
     * @return ProgressManager& The singleton instance of ProgressManager.
     */
    static ProgressManager &instance();

    /**
     * @brief Set the file name associated with the progress.
     *
     * @param fileName The name of the file being processed.
     */
    void setFileName(const QString &fileName);

    /**
     * @brief Report progress for a step.
     *
     * @param stepStart The starting point of the step as a double.
     * @param stepWeight The weight of the step as a double.
     * @param internalPercentage The internal percentage progress as an integer.
     * @param status The optional status message for the step.
     */
    void report(double stepStart, double stepWeight, int internalPercentage,
                const QString *status = nullptr);

    /**
     * @brief Request an interruption of the ongoing process.
     */
    void requestInterruption() 
    { 
        m_interruptionRequested = true; 
        emit interruptionRequested();
    }

    /**
     * @brief Check if an interruption has been requested.
     *
     * @return bool A boolean value indicating whether an interruption has been requested.
     */
    bool isInterrupted() const { return m_interruptionRequested; }

    /**
     * @brief Reset the progress manager to its initial state.
     */
    void reset();

  private:
    QString m_baseStatus = "Initializing";
    int m_dotCount = 0;
    QTimer *m_dotTimer;
    std::atomic<bool> m_interruptionRequested{false};
    QString m_fileName;

  private:
    /**
     * @brief Private constructor for the ProgressManager class.
     */
    ProgressManager();

    /**
     * @brief Animate the dot indicator for ongoing operations.
     */
    void animateDots();
};
