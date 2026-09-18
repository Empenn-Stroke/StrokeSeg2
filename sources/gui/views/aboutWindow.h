// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <QFile>
#include <QTextEdit>

/**
 * @brief This class is a window class for the application about dialog.
 *
 * This class provides a window that displays information about the application,
 * including its developers, version, license, and publications. It also allows
 * users to close the window.
 */
class AboutWindow : public QWidget {
    Q_OBJECT

  public:
    /**
     * @brief Constructor for AboutWindow.
     *
     * @param parent The parent widget for this window. Default is nullptr.
     */
    explicit AboutWindow(QWidget *parent = nullptr);

  protected:
    /**
     * @brief Reimplemented event filter to handle mouse events.
     *
     * This method overrides the default event filter to allow for window dragging
     * by clicking and dragging the title bar.
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