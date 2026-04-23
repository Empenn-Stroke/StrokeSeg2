#include "niftiViewerWindow.h"
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

// Updated constructor signature and initializer list
NiftiViewerWindow::NiftiViewerWindow(const QString &t1Path, const QString &finalPath,
                                     QWidget *parent)
    : QDialog(parent), m_t1Path(t1Path), m_finalPath(finalPath) {

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
    m_titleBar->setFixedHeight(30);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(15, 0, 10, 0);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("Adso - Anatomical Data Slice Observer", m_titleBar);

    QPushButton *closeBtn = new QPushButton("X", m_titleBar);
    closeBtn->setObjectName("closeBtn");
    closeBtn->setFixedSize(25, 25);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addWidget(closeBtn);

    // =========================================================
    //                    MAIN AREA
    // =========================================================

    QWidget *mainArea = new QWidget(this);
    mainArea->setObjectName("mainArea");
    mainArea->setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainArea);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(10);

    QHBoxLayout *bcLayout = new QHBoxLayout();
    brightnessSlider = new QSlider(Qt::Horizontal, this);
    brightnessSlider->setRange(0, 255);
    brightnessSlider->setValue(0);

    contrastSlider = new QSlider(Qt::Horizontal, this);
    contrastSlider->setRange(1, 200);
    contrastSlider->setValue(50);

    bcLayout->addWidget(new QLabel("Brightness:"));
    bcLayout->addWidget(brightnessSlider);
    bcLayout->addWidget(new QLabel("Contrast:"));
    bcLayout->addWidget(contrastSlider);

    QHBoxLayout *imagesLayout = new QHBoxLayout();

    auto createViewPanel = [this](ClickableLabel *&label, QSlider *&slider, const QString &title,
                                  int axis) {
        QVBoxLayout *layout = new QVBoxLayout();
        label = new ClickableLabel(this);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(256, 256);

        slider = new QSlider(Qt::Horizontal, this);
        slider->setEnabled(false);

        connect(label, &ClickableLabel::imageClicked, [this, axis, label](int px, int py) {
            this->handleImageClick(axis, px, py, label);
        });

        layout->addWidget(label);
        layout->addWidget(slider);
        return layout;
    };

    imagesLayout->addLayout(createViewPanel(sagittalLabel, sagittalSlider, "Sagittal (X)", 0));
    imagesLayout->addLayout(createViewPanel(coronalLabel, coronalSlider, "Coronal (Y)", 1));
    imagesLayout->addLayout(createViewPanel(axialLabel, axialSlider, "Axial (Z)", 2));

    statusLabel = new QLabel("No image loaded.", this);

    mainLayout->addLayout(bcLayout);
    mainLayout->addLayout(imagesLayout);
    mainLayout->addWidget(statusLabel);

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(mainArea);

    connect(sagittalSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(coronalSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(axialSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(brightnessSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(contrastSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);

    // =========================================================
    //               AUTO-LOAD THE VOLUME
    // =========================================================
    if (!m_t1Path.isEmpty()) {
        QTimer::singleShot(50, this, [this]() { this->loadVolume(m_t1Path, m_finalPath); });
    }
}

NiftiViewerWindow::~NiftiViewerWindow() {}

bool NiftiViewerWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_titleBar) {
        auto *e = static_cast<QMouseEvent *>(event);
        if (event->type() == QEvent::MouseButtonPress && e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        }
        if (event->type() == QEvent::MouseMove && m_dragging) {
            move(e->globalPosition().toPoint() - m_dragPosition);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
            return true;
        }
    }
    return QDialog::eventFilter(obj, event);
}

void NiftiViewerWindow::loadVolume(const QString &fileName, const QString &maskFileName) {
    if (fileName.isEmpty())
        return;

    statusLabel->setText("Loading and standardizing orientation...");
    QApplication::setOverrideCursor(Qt::WaitCursor);

    QCoreApplication::processEvents();

    try {
        // Load main volume
        myVolume = NiftiVolume::loadNiftiToRAS(fileName);
        isLoaded = true;

        // Load mask if provided
        isMaskLoaded = false;
        if (!maskFileName.isEmpty()) {
            myMaskVolume = NiftiVolume::loadNiftiToRAS(maskFileName);

            if (myVolume.getShape() == myMaskVolume.getShape()) {
                isMaskLoaded = true;
            } else {
                qDebug() << "Mask dimensions (" << myMaskVolume.getShape()
                         << ") do not match T1 dimensions ("
                         << myVolume.getShape() << "). Disabling overlay.";
            }
        }

        std::vector<int64_t> shape = myVolume.getShape();
        int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

        sagittalSlider->setRange(0, dimX - 1);
        coronalSlider->setRange(0, dimY - 1);
        axialSlider->setRange(0, dimZ - 1);

        sagittalSlider->setEnabled(true);
        coronalSlider->setEnabled(true);
        axialSlider->setEnabled(true);

        sagittalSlider->setValue(dimX / 2);
        coronalSlider->setValue(dimY / 2);
        axialSlider->setValue(dimZ / 2);

        statusLabel->setText(
            QString("Successfully loaded: %1x%2x%3").arg(dimX).arg(dimY).arg(dimZ));

        updateViews();
    } catch (const std::exception &e) {
        isLoaded = false;
        isMaskLoaded = false;
        QMessageBox::critical(this, "Error", QString("Failed to load:\n") + e.what());
        statusLabel->setText("Error loading file.");
    }
}

void NiftiViewerWindow::openNiftiFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open NIfTI Image"), "",
                                                    tr("NIfTI Files (*.nii *.nii.gz)"));
    // Manual loading unloads any mask currently applied
    loadVolume(fileName, "");
}

void NiftiViewerWindow::updateViews() {
    if (!isLoaded)
        return;

    currentX = sagittalSlider->value();
    currentY = coronalSlider->value();
    currentZ = axialSlider->value();

    QImage sagImg = extractSlice(0, currentX);
    QImage corImg = extractSlice(1, currentY);
    QImage axiImg = extractSlice(2, currentZ);

    auto drawCrosshair = [&](QImage &img, int axis) {
        QPainter p(&img);
        p.setPen(QPen(Qt::green, 1)); // Changed to green so it doesn't blend into the red mask

        std::vector<int64_t> shape = myVolume.getShape();
        int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

        if (axis == 0) {
            p.drawLine(currentY, 0, currentY, img.height());
            p.drawLine(0, (dimZ - 1) - currentZ, img.width(), (dimZ - 1) - currentZ);
        } else if (axis == 1) {
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, (dimZ - 1) - currentZ, img.width(), (dimZ - 1) - currentZ);
        } else if (axis == 2) {
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, (dimY - 1) - currentY, img.width(), (dimY - 1) - currentY);
        }
    };

    drawCrosshair(sagImg, 0);
    drawCrosshair(corImg, 1);
    drawCrosshair(axiImg, 2);

    sagittalLabel->setPixmap(QPixmap::fromImage(sagImg).scaled(
        sagittalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    coronalLabel->setPixmap(QPixmap::fromImage(corImg).scaled(
        coronalLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    axialLabel->setPixmap(QPixmap::fromImage(axiImg).scaled(axialLabel->size(), Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation));
}

QImage NiftiViewerWindow::extractSlice(int axis, int sliceIndex) {
    std::vector<int64_t> shape = myVolume.getShape();
    int nx = shape[0];
    int ny = shape[1];
    int nz = shape[2];

    int width = 0, height = 0;

    if (axis == 0) {
        width = ny;
        height = nz;
    } else if (axis == 1) {
        width = nx;
        height = nz;
    } else if (axis == 2) {
        width = nx;
        height = ny;
    }

    QImage img(width, height, QImage::Format_RGB32);

    float contrast = contrastSlider->value() / 50.0f;
    float brightness = brightnessSlider->value();

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            float val = 0.0f;
            float maskVal = 0.0f;

            if (axis == 0) {
                val = myVolume.data(sliceIndex, i, j, 0);
                if (isMaskLoaded)
                    maskVal = myMaskVolume.data(sliceIndex, i, j, 0);
            } else if (axis == 1) {
                val = myVolume.data(i, sliceIndex, j, 0);
                if (isMaskLoaded)
                    maskVal = myMaskVolume.data(i, sliceIndex, j, 0);
            } else if (axis == 2) {
                val = myVolume.data(i, j, sliceIndex, 0);
                if (isMaskLoaded)
                    maskVal = myMaskVolume.data(i, j, sliceIndex, 0);
            }

            // Base T1 pixel calculation
            int pixelVal = std::clamp(static_cast<int>((val * contrast) + brightness), 0, 255);

            int r = pixelVal;
            int g = pixelVal;
            int b = pixelVal;

            // Apply red mask overlay if the voxel has a segmentation value
            if (isMaskLoaded && maskVal > 0.1f) {
                float opacity = 0.4f; // Adjust this value (0.0 to 1.0) to change transparency

                r = std::clamp(static_cast<int>((pixelVal * (1.0f - opacity)) + (255 * opacity)), 0,
                               255);
                g = std::clamp(static_cast<int>((pixelVal * (1.0f - opacity))), 0, 255);
                b = std::clamp(static_cast<int>((pixelVal * (1.0f - opacity))), 0, 255);
            }

            img.setPixel(i, j, qRgb(r, g, b));
        }
    }

    return img.flipped(Qt::Vertical);
}

void NiftiViewerWindow::handleImageClick(int axis, int px, int py, ClickableLabel *label) {
    if (!isLoaded || label->pixmap().isNull())
        return;

    QSize pixSize = label->pixmap().size();
    int offsetX = (label->width() - pixSize.width()) / 2;
    int offsetY = (label->height() - pixSize.height()) / 2;

    double normX = std::clamp((double)(px - offsetX) / pixSize.width(), 0.0, 1.0);
    double normY = std::clamp((double)(py - offsetY) / pixSize.height(), 0.0, 1.0);

    double normYInverted = 1.0 - normY;

    std::vector<int64_t> shape = myVolume.getShape();
    int dimX = shape[0], dimY = shape[1], dimZ = shape[2];

    if (axis == 0) {
        coronalSlider->setValue(normX * (dimY - 1));
        axialSlider->setValue(normYInverted * (dimZ - 1));
    } else if (axis == 1) {
        sagittalSlider->setValue(normX * (dimX - 1));
        axialSlider->setValue(normYInverted * (dimZ - 1));
    } else if (axis == 2) {
        sagittalSlider->setValue(normX * (dimX - 1));
        coronalSlider->setValue(normYInverted * (dimY - 1));
    }
}