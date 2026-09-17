// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QMimeData>
#include <QToolButton>

/**
 * @brief The FileDropButton class is an extension of QToolButton with drag-and-drop file support.
 *
 * This class extends QToolButton to allow users to drop files onto the button.
 * When a file is dropped, it emits a signal with the file path. If an unsupported
 * file format is dropped, it emits an error signal with an appropriate message.
 */
class FileDropButton : public QToolButton 
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for FileDropButton.
     *
     * Initializes the file drop button with a specified parent widget.
     *
     * @param parent The parent widget for this button. Defaults to nullptr.
     */
    explicit FileDropButton(QWidget *parent = nullptr);

  signals:
    /**
     * @brief Signal emitted when a file is successfully dropped.
     *
     * @param filePath The file path of the dropped file.
     */
    void fileDropped(const QString &filePath);

    /**
     * @brief Signal emitted when an error occurs during file dropping.
     *
     * @param errorMessage A message describing the error.
     */
    void errorOccurred(const QString &errorMessage);

  protected:
    /**
     * @brief Handles drag enter events.
     *
     * This method is overridden to accept drag events if the mime data contains URLs.
     *
     * @param event The drag enter event that occurred.
     */
    void dragEnterEvent(QDragEnterEvent *event) override;

    /**
     * @brief Handles drag move events.
     *
     * This method is overridden to change the cursor if the mime data contains URLs.
     *
     * @param event The drag move event that occurred.
     */
    void dragMoveEvent(QDragMoveEvent *event) override;

    /**
     * @brief Handles drag leave events.
     *
     * This method is overridden to reset the cursor when the drag leaves the button.
     *
     * @param event The drag leave event that occurred.
     */
    void dragLeaveEvent(QDragLeaveEvent *event) override;

    /**
     * @brief Handles drop events.
     *
     * This method is overridden to handle file drops and emit the appropriate signals.
     *
     * @param event The drop event that occurred.
     */
    void dropEvent(QDropEvent *event) override;

  private:
    /**
     * @brief Checks if the file format is supported.
     *
     * @param filePath The file path to check.
     * @return True if the file format is supported, false otherwise.
     */
    bool isSupportedFormat(const QString &filePath) const;
};
