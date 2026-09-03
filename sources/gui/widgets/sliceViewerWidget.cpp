// SPDX-License-Identifier: AGPL-3.0-or-later

#include <widgets/sliceViewerWidget.h>
#include <QPainter>
#include <algorithm>

/**
 * @brief Constructs a SliceViewerWidget for displaying slices of a 3D volume along a specified
 * axis.
 * @param axis The axis along which to view the slices (Sagittal, Coronal, or Axial).
 * @param parent The parent widget, if any.
 */
SliceViewerWidget::SliceViewerWidget(eAxis axis, QWidget *parent) : QWidget(parent), m_axis(axis) 
{

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);

    m_imageLabel = new ClickableLabel(this);
    m_imageLabel->setAlignment(Qt::AlignCenter);
    m_imageLabel->setMinimumSize(256, 256);

    m_slider = new QSlider(Qt::Horizontal, this);
    m_slider->setEnabled(false);

    layout->addWidget(m_imageLabel);
    layout->addWidget(m_slider);

    connect(m_slider, &QSlider::valueChanged, [this](int value) { emit sliceChanged(m_axis, value); });

    connect(m_imageLabel, &ClickableLabel::imageClicked, [this](int px, int py) { emit viewClicked(m_axis, px, py, m_imageLabel); });
}

/**
 * @brief Sets up the slider range based on the maximum number of slices available for the given
 * axis.
 * @param maxSlice The maximum slice index available for the given axis.
 */
void SliceViewerWidget::setupDimensions(int maxSlice) 
{
    m_slider->setRange(0, maxSlice);
    m_slider->setEnabled(true);
}

/**
 * @brief Sets the current slice value on the slider without emitting the valueChanged signal.
 * @param value The slice index to set on the slider.
 */
void SliceViewerWidget::setSliceValue(int value) 
{
    QSignalBlocker blocker(m_slider);
    m_slider->setValue(value);
}

/**
 * @brief Returns the current slice value from the slider.
 * @return The current slice index selected on the slider.
 */
int SliceViewerWidget::getSliceValue() const 
{
    return m_slider->value();
}

/**
 * @brief Updates the displayed slice image based on the provided volume data, mask data, and
 * current slice indices. It also applies contrast and brightness adjustments.
 * @param volume The NiftiVolume object containing the 3D volume data.
 * @param isVolumeLoaded Indicates whether the volume data is loaded and valid.
 * @param maskVolume The NiftiVolume object containing the mask data (if any).
 * @param isMaskLoaded Indicates whether the mask data is loaded and valid.
 * @param currentX The current X-coordinate for slice selection.
 * @param currentY The current Y-coordinate for slice selection.
 * @param currentZ The current Z-coordinate for slice selection.
 * @param contrast The contrast adjustment factor.
 * @param brightness The brightness adjustment factor.
 */
void SliceViewerWidget::updateSlice(const NiftiVolume &volume, bool isVolumeLoaded, const NiftiVolume &maskVolume, bool isMaskLoaded, int currentX, int currentY, int currentZ, float contrast, float brightness) 
{
    if (!isVolumeLoaded) 
    {
        return;
    }

    int sliceIndex = 0;
    switch (m_axis) 
    {
    case eAxis::Sagittal:
        sliceIndex = currentX;
        break;
    case eAxis::Coronal:
        sliceIndex = currentY;
        break;
    case eAxis::Axial:
        sliceIndex = currentZ;
        break;
    }

    QImage img = extractSliceData(volume, isVolumeLoaded, maskVolume, isMaskLoaded, sliceIndex, contrast, brightness);

    drawCrosshair(img, volume.getShape(), currentX, currentY, currentZ);

    m_imageLabel->setPixmap(QPixmap::fromImage(img.mirrored(false, true)).scaled(m_imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

/**
 * @brief Extracts a 2D slice from the 3D volume data based on the specified axis and slice index.
 * It applies contrast and brightness adjustments, and overlays mask data if available.
 * @param volume The NiftiVolume object containing the 3D volume data.
 * @param isVolumeLoaded Indicates whether the volume data is loaded and valid.
 * @param maskVolume The NiftiVolume object containing the mask data (if any).
 * @param isMaskLoaded Indicates whether the mask data is loaded and valid.
 * @param sliceIndex The index of the slice to extract along the specified axis.
 * @param contrast The contrast adjustment factor.
 * @param brightness The brightness adjustment factor.
 * @return The extracted 2D slice image.
 */
QImage SliceViewerWidget::extractSliceData(const NiftiVolume &volume, bool isVolumeLoaded, const NiftiVolume &maskVolume, bool isMaskLoaded, int sliceIndex, float contrast, float brightness)
{
    std::vector<int64_t> shape = volume.getShape();
    int nx = shape[0], ny = shape[1], nz = shape[2];

    int width = (m_axis == eAxis::Sagittal) ? ny : nx;
    int height = (m_axis == eAxis::Axial) ? ny : nz;

    QImage img(width, height, QImage::Format_RGB32);

    for (int j = 0; j < height; ++j) 
    {
        for (int i = 0; i < width; ++i) 
        {
            float val = 0.0f;
            float maskVal = 0.0f;

            if (m_axis == eAxis::Sagittal) 
            {
                val = volume.data(sliceIndex, i, j, 0);
                if (isMaskLoaded)
                {
                    maskVal = maskVolume.data(sliceIndex, i, j, 0);
                }
            } 
            else if (m_axis == eAxis::Coronal) 
            {
                val = volume.data(i, sliceIndex, j, 0);
                if (isMaskLoaded)
                {
                    maskVal = maskVolume.data(i, sliceIndex, j, 0);
                }
            } 
            else if (m_axis == eAxis::Axial) 
            {
                val = volume.data(i, j, sliceIndex, 0);
                if (isMaskLoaded)
                {
                    maskVal = maskVolume.data(i, j, sliceIndex, 0);
                }
            }

            int pixelVal = 0;
            if (val != 0.0f) {
                pixelVal = std::clamp(static_cast<int>((val * contrast) + brightness), 0, 255);
            }

            int r = pixelVal, g = pixelVal, b = pixelVal;

            if (isMaskLoaded && maskVal > 0.1f) {
                float opacity = 0.4f;
                r = std::clamp(static_cast<int>((pixelVal * (1.0f - opacity)) + (255 * opacity)), 0, 255);
                g = std::clamp(static_cast<int>((pixelVal * (1.0f - opacity))), 0, 255);
                b = std::clamp(static_cast<int>((pixelVal * (1.0f - opacity))), 0, 255);
            }
            img.setPixel(i, j, qRgb(r, g, b));
        }
    }
    return img;
}

/**
 * @brief Draws a crosshair on the provided image at the specified current slice coordinates. The
 * crosshair is drawn in green color and is oriented based on the viewing axis.
 * @param img The QImage on which to draw the crosshair.
 * @param shape The shape of the 3D volume data, used to determine the dimensions for drawing the
 * crosshair.
 * @param currentX The x-coordinate of the current slice.
 * @param currentY The y-coordinate of the current slice.
 * @param currentZ The z-coordinate of the current slice.
 */
void SliceViewerWidget::drawCrosshair(QImage &img, const std::vector<int64_t> &shape, int currentX, int currentY, int currentZ)
{
    QPainter p(&img);
    p.setPen(QPen(Qt::green, 1));

    if (m_axis == eAxis::Sagittal) 
    {
        p.drawLine(currentY, 0, currentY, img.height());
        p.drawLine(0, currentZ, img.width(), currentZ);
    } 
    else if (m_axis == eAxis::Coronal) 
    {
        p.drawLine(currentX, 0, currentX, img.height());
        p.drawLine(0, currentZ, img.width(), currentZ);
    } 
    else if (m_axis == eAxis::Axial) 
    {
        p.drawLine(currentX, 0, currentX, img.height());
        p.drawLine(0, currentY, img.width(), currentY);
    }
}
