// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "clickableLabel.h"
#include "niftiVolume.h"
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief The SliceViewerWidget class is a QWidget that displays a single slice of a 3D volume
 * along a specified axis (sagittal, coronal, or axial). It provides a slider to navigate through
 * the slices and emits signals when the slice changes or when the view is clicked.
 */
class SliceViewerWidget : public QWidget 
{
    Q_OBJECT

  public:
    /**
     * @brief Enumerates the possible axes for slicing.
     */
    enum class eAxis { Sagittal = 0, Coronal = 1, Axial = 2 };
    Q_ENUM(eAxis) // Allows to use enum with qt slots and signals

    /**
     * @brief Constructor for SliceViewerWidget.
     *
     * Initializes the slice viewer widget with a specified axis and parent widget.
     *
     * @param axis The axis along which the slice should be displayed.
     * @param parent The parent widget for this widget. Defaults to nullptr.
     */
    SliceViewerWidget(eAxis axis, QWidget *parent = nullptr);

    /**
     * @brief Sets up the dimensions of the slider based on the maximum number of slices.
     *
     * @param maxSlice The maximum number of slices along the specified axis.
     */
    void setupDimensions(int maxSlice);

    /**
     * @brief Sets the current slice value.
     *
     * @param value The new slice value.
     */
    void setSliceValue(int value);

    /**
     * @brief Gets the current slice value.
     *
     * @return The current slice value.
     */
    int getSliceValue() const;

    /**
     * @brief Updates the displayed slice with new volume data.
     *
     * @param volume The NiftiVolume containing the 3D data.
     * @param isVolumeLoaded A flag indicating if the volume is loaded.
     * @param maskVolume The NiftiVolume containing the mask data.
     * @param isMaskLoaded A flag indicating if the mask is loaded.
     * @param currentX The current x-coordinate in the 3D volume.
     * @param currentY The current y-coordinate in the 3D volume.
     * @param currentZ The current z-coordinate in the 3D volume.
     * @param contrast The contrast adjustment for the slice.
     * @param brightness The brightness adjustment for the slice.
     */
    void updateSlice(const NiftiVolume &volume, bool isVolumeLoaded, const NiftiVolume &maskVolume, bool isMaskLoaded, int currentX, int currentY, int currentZ, float contrast, float brightness);

  signals:
    /**
     * @brief Signal emitted when the slice value changes due to user interaction with the slider.
     *
     * @param axis The axis along which the slice changed (sagittal, coronal, or axial).
     * @param value The new slice value.
     */
    void sliceChanged(eAxis axis, int value);

    /**
     * @brief Signal emitted when the user clicks on the displayed slice image.
     *
     * Provides the axis, click coordinates, and the label that was clicked.
     *
     * @param axis The axis along which the click occurred (sagittal, coronal, or axial).
     * @param px The x-coordinate of the click.
     * @param py The y-coordinate of the click.
     * @param label The label that was clicked.
     */
    void viewClicked(eAxis axis, int px, int py, ClickableLabel *label);

  private:
    /**
     * @brief Extracts the data for a single slice from the NiftiVolume.
     *
     * @param volume The NiftiVolume containing the 3D data.
     * @param isVolumeLoaded A flag indicating if the volume is loaded.
     * @param maskVolume The NiftiVolume containing the mask data.
     * @param isMaskLoaded A flag indicating if the mask is loaded.
     * @param sliceIndex The index of the slice to extract.
     * @param contrast The contrast adjustment for the slice.
     * @param brightness The brightness adjustment for the slice.
     * @return The QImage containing the slice data.
     */
    QImage extractSliceData(const NiftiVolume &volume, bool isVolumeLoaded, const NiftiVolume &maskVolume, bool isMaskLoaded, int sliceIndex, float contrast, float brightness);

    /**
     * @brief Draws a crosshair on the image at the specified coordinates.
     *
     * @param img The QImage on which to draw the crosshair.
     * @param shape The shape of the 3D volume.
     * @param currentX The current x-coordinate in the 3D volume.
     * @param currentY The current y-coordinate in the 3D volume.
     * @param currentZ The current z-coordinate in the 3D volume.
     */
    void drawCrosshair(QImage &img, const std::vector<int64_t> &shape, int currentX, int currentY, int currentZ);

private:
    eAxis m_axis; /**< The axis along which the slice is displayed. */
    ClickableLabel *m_imageLabel; /**< The label displaying the slice image. */
    QSlider *m_slider; /**< The slider for navigating through slices. */
};
