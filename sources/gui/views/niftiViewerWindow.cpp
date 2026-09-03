// SPDX-License-Identifier: AGPL-3.0-or-later

#include "niftiViewerWindow.h"
#include <preprocessing/preprocessor.h>
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QString>
#include <QTimer>
#include <algorithm>

/**
 * @brief Constructs a NiftiViewerWindow for displaying 3D NIfTI volumes and their corresponding
 * masks.
 * @param t1Path The file path to the T1-weighted NIfTI volume to be loaded.
 * @param finalPath The file path to the final NIfTI volume to be loaded.
 * @param parent The parent widget, if any.
 */
NiftiViewerWindow::NiftiViewerWindow(const QString &t1Path, const QString &finalPath, QWidget *parent)
    : QDialog(parent), m_t1Path(t1Path), m_finalPath(finalPath),
      m_viewers{SliceViewerWidget(SliceViewerWidget::eAxis::Sagittal, this), SliceViewerWidget(SliceViewerWidget::eAxis::Coronal, this), SliceViewerWidget(SliceViewerWidget::eAxis::Axial, this)} 
{

    setWindowTitle("Adso - Anatomical Data Slice Observer");
    resize(900, 550);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    this->setObjectName("niftiViewerWindow");

    QVBoxLayout *windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor(25, 60, 105, 30));
    this->setGraphicsEffect(shadow);

    // =========================================================
    //                      TITLE BAR
    // =========================================================

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(25);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(15, 0, 0, 0);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("Adso - Anatomical Data Slice Observer", m_titleBar);

    QPushButton *closeBtn = new QPushButton("\u00D7", m_titleBar);
    closeBtn->setObjectName("closeBtn");
    closeBtn->setFixedSize(30, 25);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);

    // =========================================================
    //                    MAIN AREA
    // =========================================================

    QWidget *mainArea = new QWidget(this);
    mainArea->setAttribute(Qt::WA_StyledBackground, true);
    mainArea->setObjectName("adsoMainArea");

    QVBoxLayout *mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    QHBoxLayout *bcLayout = new QHBoxLayout();
    brightnessSlider = new QSlider(Qt::Horizontal, this);
    brightnessSlider->setRange(0, 255);
    brightnessSlider->setValue(0);
    brightnessSlider->setObjectName("adsoBrightnessSlider");

    contrastSlider = new QSlider(Qt::Horizontal, this);
    contrastSlider->setRange(1, 200);
    contrastSlider->setValue(50);
    contrastSlider->setObjectName("adsoContrastSlider");

    bcLayout->addWidget(new QLabel("Brightness:"));
    bcLayout->addWidget(brightnessSlider);
    bcLayout->addWidget(new QLabel("Contrast:"));
    bcLayout->addWidget(contrastSlider);

    QHBoxLayout *imagesLayout = new QHBoxLayout();

    SliceViewerWidget::eAxis axes[] = {SliceViewerWidget::eAxis::Sagittal,
                                      SliceViewerWidget::eAxis::Coronal,
                                      SliceViewerWidget::eAxis::Axial};

    for (int i = 0; i < 3; ++i) 
    {
        imagesLayout->addWidget(&m_viewers[i]);

        connect(&m_viewers[i], &SliceViewerWidget::sliceChanged, this,
                &NiftiViewerWindow::updateViews);

        connect(&m_viewers[i], &SliceViewerWidget::viewClicked, this,
                &NiftiViewerWindow::handleImageClick);
    }

    statusLabel = new QLabel("No image loaded.", this);

    mainLayout->addLayout(bcLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addWidget(statusLabel);

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(mainArea);

    connect(brightnessSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(contrastSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);

    // =========================================================
    //               AUTO-LOAD THE VOLUME
    // =========================================================
    if (!m_t1Path.isEmpty()) 
    {
        QTimer::singleShot(50, this, [this]() { this->loadVolume(m_t1Path, m_finalPath); });
    }
}

/**
 * @brief Destructor for the NiftiViewerWindow class. Cleans up any resources used by the window.
 */
NiftiViewerWindow::~NiftiViewerWindow() {}

bool NiftiViewerWindow::eventFilter(QObject *obj, QEvent *event) 
{
    if (obj == m_titleBar) 
    {
        auto *e = static_cast<QMouseEvent *>(event);
        if (event->type() == QEvent::MouseButtonPress && e->button() == Qt::LeftButton) 
        {
            m_dragging = true;
            m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        }
        if (event->type() == QEvent::MouseMove && m_dragging) 
        {
            move(e->globalPosition().toPoint() - m_dragPosition);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) 
        {
            m_dragging = false;
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

/**
 * @brief Loads a NIfTI volume and an optional mask volume, standardizes their orientation, and
 * updates the viewer windows.
 * @param fileName The file path to the NIfTI volume to be loaded.
 * @param maskFileName The file path to the optional mask NIfTI volume to be loaded. If empty, no
 * mask will be loaded.
 */
void NiftiViewerWindow::loadVolume(const QString &fileName, const QString &maskFileName) 
{
    if (fileName.isEmpty())
        return;

    statusLabel->setText("Loading and standardizing orientation...");
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QCoreApplication::processEvents();

    try 
    {
        myVolume = NiftiVolume::loadNiftiToRAS(fileName);
        isLoaded = true;

        // Normalization
        statusLabel->setText("Normalizing volume intensities...");
        QCoreApplication::processEvents();

        myNormalizedVolume = myVolume;

        preprocessing::Preprocessor::zScoreNormalize(myNormalizedVolume, nullptr);
        preprocessing::Preprocessor::minMaxNormalize(myNormalizedVolume, 0, 255);

        isMaskLoaded = false;
        if (!maskFileName.isEmpty()) 
        {
            myMaskVolume = NiftiVolume::loadNiftiToRAS(maskFileName);

            if (myVolume.getShape() == myMaskVolume.getShape()) 
            {
                isMaskLoaded = true;
            } else 
            {
                qDebug() << "Mask dimensions (" << myMaskVolume.getShape()<< ") do not match T1 dimensions (" << myVolume.getShape() << "). Disabling overlay.";
            }
        }

        std::vector<int64_t> shape = myVolume.getShape();
        for (int i = 0; i < 3; ++i) 
        {
            m_viewers[i].setupDimensions(shape[i] - 1);
            m_viewers[i].setSliceValue(shape[i] / 2);
        }

        statusLabel->setText(QString("Successfully loaded: %1x%2x%3").arg(shape[0]).arg(shape[1]).arg(shape[2]));
        updateViews();
        QApplication::restoreOverrideCursor();

    } catch (const std::exception &e) 
    {
        isLoaded = false;
        isMaskLoaded = false;
        QMessageBox::critical(this, "Error", QString("Failed to load:\n") + e.what());
        statusLabel->setText("Error loading file.");
    }
}

/**
 * @brief Opens a file dialog to select a NIfTI file and loads it into the viewer.
 */
void NiftiViewerWindow::openNiftiFile() 
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open NIfTI Image"), "", tr("NIfTI Files (*.nii *.nii.gz)"));
    loadVolume(fileName, "");
}

/**
 * @brief Updates the displayed slices in all three viewer windows based on the current slice values
 * and the brightness and contrast settings.
 */
void NiftiViewerWindow::updateViews() 
{
    if (!isLoaded) 
    {
        return;
    }

    currentX = m_viewers[0].getSliceValue();
    currentY = m_viewers[1].getSliceValue();
    currentZ = m_viewers[2].getSliceValue();

    float contrast = contrastSlider->value() / 50.0f;
    float brightness = brightnessSlider->value();

    for (int i = 0; i < 3; ++i) 
    {
        m_viewers[i].updateSlice(myNormalizedVolume, isLoaded, myMaskVolume, isMaskLoaded, currentX, currentY, currentZ, contrast, brightness);
    }
}

/**
 * @brief Handles a click event on one of the slice viewer widgets. It calculates the normalized 
 * coordinates and updates the corresponding slice values.
 * @param axis The axis along which the click occurred.
 * @param px The x-coordinate of the click event.
 * @param py The y-coordinate of the click event.
 * @param label The label widget that received the click event.
 */
void NiftiViewerWindow::handleImageClick(SliceViewerWidget::eAxis axis, int px, int py, ClickableLabel *label) 
{
    if (isLoaded && !label->pixmap().isNull()) 
    {
        QSize pixSize = label->pixmap().size();
        int offsetX = (label->width() - pixSize.width()) / 2;
        int offsetY = (label->height() - pixSize.height()) / 2;

        double normX = std::clamp((double)(px - offsetX) / pixSize.width(), 0.0, 1.0);
        double normY = std::clamp((double)(py - offsetY) / pixSize.height(), 0.0, 1.0);

        std::vector<int64_t> shape = myVolume.getShape();
        int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

        double correctedY = 1.0 - normY;

        switch (axis) {
        case SliceViewerWidget::eAxis::Sagittal:
            m_viewers[1].setSliceValue(normX * (dimY - 1));
            m_viewers[2].setSliceValue(correctedY * (dimZ - 1));
            break;
        case SliceViewerWidget::eAxis::Coronal:
            m_viewers[0].setSliceValue(normX * (dimX - 1));
            m_viewers[2].setSliceValue(correctedY * (dimZ - 1));
            break;
        case SliceViewerWidget::eAxis::Axial:
            m_viewers[0].setSliceValue(normX * (dimX - 1));
            m_viewers[1].setSliceValue(correctedY * (dimY - 1));
            break;
        }

        updateViews();
    }
}
