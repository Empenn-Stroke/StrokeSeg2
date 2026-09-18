// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

#include <QTextEdit>
#include <QFile>

/**
 * @brief The GuideWindow class is a GUI window that displays guidance or instructions to the user.
 *
 * This class creates a window with a title bar and content area that can be used to display
 * instructional text or guides. The window supports dragging and includes a title bar for
 * user interaction.
 */
class GuideWindow : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for GuideWindow.
     *
     * Initializes the guide window with a specified parent widget.
     *
     * @param parent The parent widget for this window. Defaults to nullptr.
     */
    explicit GuideWindow(QWidget *parent = nullptr);

protected:
    /**
     * @brief Handles custom events for the window.
     *
     * This method is overridden to provide custom behavior for handling events.
     * It supports dragging the window by clicking and dragging the title bar.
     *
     * @param obj The object that received the event.
     * @param event The event that occurred.
     * @return True if the event was handled, false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    bool m_dragging = false; /**< A boolean indicating whether the window is being dragged. */
    QPoint m_dragPosition; /**< The position where the mouse was clicked for dragging. */

private:
    QWidget *m_titleBar; /**< The title bar widget for window dragging. */
};
