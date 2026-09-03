#pragma once

#include <QFile>
#include <QTextEdit>
#include <QTimer>
#include <QWidget>

/**
 * @brief The ConsoleWindow class provides a GUI window that displays log messages in real-time. It reads from a log file and updates the display as new log entries are added. The window is designed to be read-only and uses a monospace font for
 * better readability of log messages.
 */
class ConsoleWindow : public QWidget 
{
    Q_OBJECT

  public:
    explicit ConsoleWindow(QWidget *parent = nullptr);
    ~ConsoleWindow();

  protected:
    void closeEvent(QCloseEvent *event) override;

  private slots:
    void checkForUpdates();

  private:
    QTextEdit *m_textEdit;
    QTimer *m_updateTimer;
    QFile m_logFile;
    qint64 m_lastPosition;

    void loadExistingLogs();
};
