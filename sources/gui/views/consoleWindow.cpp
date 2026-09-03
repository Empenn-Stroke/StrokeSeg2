#include "consoleWindow.h"
#include <QFontDatabase>
#include <QScrollBar>
#include <QVBoxLayout>
#include <managers/logManager.h>

/**
 * @brief Constructs a ConsoleWindow object with the given parent widget.
 * @param parent The parent widget.
 */
ConsoleWindow::ConsoleWindow(QWidget *parent) : QWidget(parent), m_lastPosition(0) 
{
    setWindowTitle("StrokeSeg2 - Console");
    resize(800, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    m_textEdit = new QTextEdit(this);
    m_textEdit->setReadOnly(true);
    m_textEdit->setUndoRedoEnabled(false);

    QFont monoFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monoFont.setPointSize(10);
    m_textEdit->setFont(monoFont);

    layout->addWidget(m_textEdit);

    QString logPath = LogManager::getLogFilePath();
    m_logFile.setFileName(logPath);

    loadExistingLogs();

    m_updateTimer = new QTimer(this);
    connect(m_updateTimer, &QTimer::timeout, this, &ConsoleWindow::checkForUpdates);
    m_updateTimer->start(250);
}

/**
 * @brief Destructor for the ConsoleWindow class. Ensures that the log file is closed when the console window is destroyed.
 */
ConsoleWindow::~ConsoleWindow() 
{
    if (m_logFile.isOpen()) 
    {
        m_logFile.close();
    }
}

/**
 * @brief Handles the close event for the console window. It ensures that the log file is closed and the timer is stopped when the window is closed.
 */
void ConsoleWindow::loadExistingLogs() 
{
    if (m_logFile.open(QIODevice::ReadOnly | QIODevice::Text)) 
    {
        QTextStream in(&m_logFile);
        m_textEdit->setPlainText(in.readAll());

        m_lastPosition = m_logFile.pos();

        m_textEdit->verticalScrollBar()->setValue(m_textEdit->verticalScrollBar()->maximum());
    }
}

/**
 * @brief Checks for updates in the log file and appends any new content to the text edit widget. It handles cases where the log file has been truncated or recreated.
 */
void ConsoleWindow::checkForUpdates() {
    bool shouldProcess = true;

    if (!m_logFile.isOpen()) {
        if (!m_logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            shouldProcess = false;
        }
    }

    if (shouldProcess) {
        if (m_logFile.size() < m_lastPosition) {
            m_lastPosition = 0;
            m_textEdit->clear();
        }

        if (m_logFile.size() > m_lastPosition) {
            m_logFile.seek(m_lastPosition);
            QTextStream in(&m_logFile);

            QTextCursor cursor = m_textEdit->textCursor();
            cursor.movePosition(QTextCursor::End);

            cursor.insertText(in.readAll());
            m_lastPosition = m_logFile.pos();

            QScrollBar *bar = m_textEdit->verticalScrollBar();
            bar->setValue(bar->maximum());
        }
    }
}

/**
 * @brief Handles the close event for the console window. Instead of closing the window, it hides it, allowing it to be shown again later without needing to recreate it. It also ensures that the base class's close event is called.
 * @param event The close event that triggered this function.
 */
void ConsoleWindow::closeEvent(QCloseEvent *event) 
{
    QWidget::closeEvent(event);
}
