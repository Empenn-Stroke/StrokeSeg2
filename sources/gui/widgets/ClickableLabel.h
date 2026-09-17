// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QMouseEvent>

/**
 * @brief The ClickableLabel class provides a label that emits a signal when clicked.
 *
 * This class extends QLabel and emits a signal whenever the label is clicked, along
 * with the coordinates of the click. This can be useful for creating interactive
 * GUI elements where clicking on a label needs to trigger specific actions.
 */
class ClickableLabel : public QLabel {
    Q_OBJECT

public:
    /**
     * @brief Constructor for ClickableLabel.
     *
     * Initializes the clickable label with a specified parent widget.
     *
     * @param parent The parent widget for this label. Defaults to nullptr.
     */
    explicit ClickableLabel(QWidget* parent = nullptr);

signals:
    /**
     * @brief Signal emitted when the label is clicked.
     *
     * @param x The x-coordinate of the click.
     * @param y The y-coordinate of the click.
     */
    void imageClicked(int x, int y);

protected:
    /**
     * @brief Handles mouse press events.
     *
     * This method is overridden to emit the imageClicked signal when the label is clicked.
     *
     * @param event The mouse event that occurred.
     */
    void mousePressEvent(QMouseEvent* event) override;

    /**
     * @brief Handles mouse move events.
     *
     * This method is overridden to provide additional functionality for mouse movements.
     *
     * @param event The mouse event that occurred.
     */
    void mouseMoveEvent(QMouseEvent* event) override;
};

#endif // CLICKABLELABEL_H