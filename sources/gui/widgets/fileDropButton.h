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
 * @brief A custom QToolButton that supports drag-and-drop functionality for files. 
 */
class FileDropButton : public QToolButton 
{
    Q_OBJECT

  public:
    explicit FileDropButton(QWidget *parent = nullptr);

  signals:
    void fileDropped(const QString &filePath);
    void errorOccurred(const QString &errorMessage);

  protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

  private:
    bool isSupportedFormat(const QString &filePath) const;
};
