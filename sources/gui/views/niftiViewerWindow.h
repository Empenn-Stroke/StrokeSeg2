// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <widgets/sliceViewerWidget.h>
#include "niftiVolume.h"
#include <QDialog>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QPushButton>
#include <QSlider>
#include <QString>
#include <QVBoxLayout>

/**
 * @brief The NiftiViewerWindow class is a GUI dialog for viewing NIfTI volumes.
 *
 * This class creates a dialog window that allows users to load and view NIfTI volumes,
 * including optional mask volumes. It supports window dragging, slice visualization,
 * and basic interaction with the volume data.
 */
class NiftiViewerWindow : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for NiftiViewerWindow.
     *
     * Initializes the NIfTI viewer dialog with specified base and mask file paths.
     *
     * @param t1Path The file path of the base NIfTI volume.
     * @param finalPath The file path of the mask NIfTI volume.
     * @param parent The parent widget for this dialog. Defaults to nullptr.
     */
    explicit NiftiViewerWindow(const QString &t1Path, const QString &finalPath, QWidget *parent = nullptr);
    ~NiftiViewerWindow();

    /**
     * @brief Loads the NIfTI volume and optional mask volume.
     *
     * @param fileName The file path of the NIfTI volume to be loaded.
     * @param maskFileName The file path of the mask NIfTI volume. Defaults to an empty string.
     */
    void loadVolume(const QString &fileName, const QString &maskFileName = "");

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

  private slots:
    /**
     * @brief Opens a NIfTI file.
     */
    void openNiftiFile();

    /**
     * @brief Updates the slice viewers.
     */
    void updateViews();

    /**
     * @brief Handles image click events.
     *
     * @param axis The axis of the slice viewer.
     * @param px The x-coordinate of the click.
     * @param py The y-coordinate of the click.
     * @param label The clickable label associated with the slice viewer.
     */
    void handleImageClick(SliceViewerWidget::eAxis axis, int px, int py, ClickableLabel *label);

  private:
    // --- Window Dragging State ---
    QWidget *m_titleBar; /**< The title bar widget for window dragging. */
    bool m_dragging = false; /**< A boolean indicating whether the window is being dragged. */
    QPoint m_dragPosition; /**< The position where the mouse was clicked for dragging. */

    // --- File Paths ---
    QString m_t1Path; /**< The file path of the base NIfTI volume. */
    QString m_finalPath; /**< The file path of the mask NIfTI volume. */

    // --- UI Elements ---
    QLabel *statusLabel; /**< A label for displaying status messages. */
    std::array<SliceViewerWidget, 3> m_viewers; /**< Three slice viewer widgets for different axes. */
    QSlider *brightnessSlider; /**< A slider for adjusting the brightness of the slice views. */
    QSlider *contrastSlider; /**< A slider for adjusting the contrast of the slice views. */

    // --- State Variables ---
    NiftiVolume myVolume; /**< The main NIfTI volume data. */
    NiftiVolume myNormalizedVolume; /**< The normalized NIfTI volume data. */
    bool isLoaded = false; /**< A boolean indicating whether the volume is loaded. */

    // --- Mask Variables ---
    NiftiVolume myMaskVolume; /**< The mask NIfTI volume data. */
    bool isMaskLoaded = false; /**< A boolean indicating whether the mask is loaded. */

    int currentX = 0; /**< The current x-coordinate in the volume. */
    int currentY = 0; /**< The current y-coordinate in the volume. */
    int currentZ = 0; /**< The current z-coordinate in the volume. */

    /**
     * @brief Extracts a slice from the volume.
     *
     * @param axis The axis of the slice (0: X, 1: Y, 2: Z).
     * @param sliceIndex The index of the slice along the specified axis.
     * @return A QImage containing the extracted slice.
     */
    QImage extractSlice(int axis, int sliceIndex);
};
