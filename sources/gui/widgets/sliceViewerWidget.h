// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include "clickableLabel.h"
#include "niftiVolume.h"
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

/**
 * @brief SliceViewerWidget is a QWidget that displays a single slice of a 3D volume along a
 * specified axis (sagittal, coronal, or axial). It provides a slider to navigate through the slices 
 * and emits signals when the slice changes or when the view is clicked.
 */
class SliceViewerWidget : public QWidget 
{
    Q_OBJECT

  public:
    enum class eAxis { Sagittal = 0, Coronal = 1, Axial = 2 };
    Q_ENUM(eAxis) // Allows to use enum with qt slots and signals

    SliceViewerWidget(eAxis axis, QWidget *parent = nullptr);

    void setupDimensions(int maxSlice);
    void setSliceValue(int value);
    int getSliceValue() const;

    void updateSlice(const NiftiVolume &volume, bool isVolumeLoaded, const NiftiVolume &maskVolume, bool isMaskLoaded, int currentX, int currentY, int currentZ, float contrast, float brightness);

  signals:
    /**
     * @brief Emitted when the slice value changes due to user interaction with the slider. 
     * @param axis The axis along which the slice changed (sagittal, coronal, or axial).
     * @param value The new slice value.
     */
    void sliceChanged(eAxis axis, int value);

    /**
     * @brief Emitted when the user clicks on the displayed slice image. Provides the axis, click coordinates, and the label that was clicked.
     * @param axis The axis along which the click occurred (sagittal, coronal, or axial).
     * @param px The x-coordinate of the click.
     * @param py The y-coordinate of the click.
     * @param label The label that was clicked.
     */
    void viewClicked(eAxis axis, int px, int py, ClickableLabel *label);

  private:
    QImage extractSliceData(const NiftiVolume &volume, bool isVolumeLoaded, const NiftiVolume &maskVolume, bool isMaskLoaded, int sliceIndex, float contrast, float brightness);

    void drawCrosshair(QImage &img, const std::vector<int64_t> &shape, int currentX, int currentY, int currentZ);

  private:
    eAxis m_axis;
    ClickableLabel *m_imageLabel;
    QSlider *m_slider;
};
