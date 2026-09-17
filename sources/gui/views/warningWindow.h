// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialog>
#include <QWidget>

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <QTextEdit>
#include <QCheckBox>

#include <QSettings>

/**
 * @brief The WarningWindow class provides a GUI dialog for displaying warning messages.
 *
 * This class creates a dialog window that shows warning messages and an option to
 * not show the warning again in the future. The window supports window dragging and
 * includes a title bar for user interaction.
 */
class WarningWindow : public QDialog {
    Q_OBJECT

  public:
    /**
     * @brief Constructor for WarningWindow.
     *
     * Initializes the warning dialog with a specified parent widget.
     *
     * @param parent The parent widget for this dialog. Defaults to nullptr.
     */
    explicit WarningWindow(QDialog *parent = nullptr);

  protected:
    /**
     * @brief Handles custom events for the dialog.
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
    QCheckBox *m_dontShowAgain; /**< A checkbox for the user to choose not to show the warning again. */
};