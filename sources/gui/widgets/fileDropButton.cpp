// SPDX-License-Identifier: AGPL-3.0-or-later

#include "fileDropButton.h"

#include <QDir>
#include <QFileInfo>
#include <QStyle>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
/**
 * @brief Constructs a FileDropButton object with the given parent widget.
 * @param parent The parent widget.
 */
FileDropButton::FileDropButton(QWidget *parent) : QToolButton(parent)
{
    setAcceptDrops(true);
}

/**
 * @brief Handles the drag enter event when a file is dragged over the button.
 * @param event The drag enter event.
 */
void FileDropButton::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        setProperty("dragging", true);
        style()->unpolish(this);
        style()->polish(this);
        update();
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief Handles the drag move event when a file is dragged over the button.
 * @param event The drag move event.
 */
void FileDropButton::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }
}

/**
 * @brief Handles the drag leave event when a file is dragged away from the button.
 * @param event The drag leave event.
 */
void FileDropButton::dragLeaveEvent(QDragLeaveEvent *event)
{
    setProperty("dragging", false);
    style()->unpolish(this);
    style()->polish(this);
    update();
    event->accept();
}

/**
 * @brief Handles the drop event when a file is dropped onto the button.
 * @param event The drop event.
 */
void FileDropButton::dropEvent(QDropEvent *event)
{
    const QList<QUrl> urls = event->mimeData()->urls();

    if (!urls.isEmpty())
    {
        QString filePath = QDir::cleanPath(urls.first().toLocalFile());

        qDebug() << "[DROP] Fichier détecté brut :" << urls.first().toString();
        qDebug() << "[DROP] Chemin local nettoyé :" << filePath;

        if (isSupportedFormat(filePath))
        {
            qDebug() << "[DROP] Format validé ! Émission de fileDropped pour :" << filePath;
            event->acceptProposedAction();
            emit fileDropped(filePath);
        }
        else
        {
            qDebug() << "[DROP] [ERREUR] Format REJETÉ pour :" << filePath;
            emit errorOccurred("Unsupported format");
            event->ignore();
        }
    }

    setProperty("dragging", false);
    style()->unpolish(this);
    style()->polish(this);
    update();
}

/**
 * @brief Checks if the given file path has a supported format.
 * @param filePath The file path to check.
 * @return True if the file format is supported, false otherwise.
 */
bool FileDropButton::isSupportedFormat(const QString &filePath) const
{
    if (filePath.isEmpty())
    {
        return false;
    }

    QFileInfo info(filePath);
    if (info.isDir())
    {
        qDebug() << "[DROP INFO] C'est un répertoire.";
        return true;
    }

    static const QStringList supportedExtensions = {"nii", "nii.gz", "nrrd", "dcm"};
    QString suffix = info.suffix().toLower();
    QString completeSuffix = info.completeSuffix().toLower();

    qDebug() << "[DROP INFO] Suffix :" << suffix << "| CompleteSuffix :" << completeSuffix;

    if (supportedExtensions.contains(suffix) || supportedExtensions.contains(completeSuffix))
    {
        return true;
    }

    for (const QString &ext : supportedExtensions)
    {
        if (filePath.toLower().endsWith("." + ext))
        {
            return true;
        }
    }

    return false;
}
