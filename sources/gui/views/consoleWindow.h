#pragma once

#include <QFile>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

/**
 * @brief The ConsoleWindow class provides a GUI window that displays log messages in real-time.
 *
 * This class creates a window that reads from a log file and updates the display as new log entries are added.
 * The window is designed to be read-only and uses a monospace font for better readability of log messages.
 */
class ConsoleWindow : public QWidget 
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for ConsoleWindow.
     *
     * Initializes the console window with a specified parent widget.
     *
     * @param parent The parent widget for this window. Defaults to nullptr.
     */
    explicit ConsoleWindow(QWidget *parent = nullptr);

    /**
     * @brief Destructor for ConsoleWindow.
     */
    ~ConsoleWindow();

  protected:
    /**
     * @brief Handles the close event for the window.
     *
     * This method is overridden to provide custom behavior when the window is closed.
     *
     * @param event The close event.
     */
    void closeEvent(QCloseEvent *event) override;

  private slots:
    /**
     * @brief Checks for and loads new log updates.
     *
     * This slot is connected to the timer and is called periodically to check for new log entries
     * in the log file and update the display accordingly.
     */
    void checkForUpdates();

private:
    QTextEdit *m_textEdit; /**< The text edit widget for displaying log messages. */
    QTimer *m_updateTimer; /**< The timer for periodic log updates. */
    QFile m_logFile; /**< The log file from which to read messages. */
    qint64 m_lastPosition; /**< The last read position in the log file. */

    /**
     * @brief Loads existing log entries from the log file.
     *
     * This method reads the existing log entries from the log file and displays them
     * in the text edit widget when the console window is first opened.
     */
    void loadExistingLogs();
};
