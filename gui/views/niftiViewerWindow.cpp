#include "NiftiViewerWindow.h"
#include <QMessageBox>
#include <algorithm>

NiftiViewerWindow::NiftiViewerWindow(const QString &brainPath, const QString &maskPath,
                                     QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("StrokeSeg2 - Viewer");
    resize(1000, 500);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    QWidget *m_mainWidget = new QWidget(this);
    m_mainWidget->setObjectName("m_mainWidget");
    m_mainWidget->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(m_mainWidget);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor(25, 60, 105, 30));
    m_mainWidget->setGraphicsEffect(shadow);

    QVBoxLayout *windowLayout = new QVBoxLayout(m_mainWidget);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);

    // =========================================================
    //                      TITLE BAR
    // =========================================================

    m_titleBar = new QWidget(m_mainWidget);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(25);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(60, 0, 0, 2);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("Lesion Viewer", m_titleBar);
    QPushButton *reduceBtn = new QPushButton("\u2212", m_titleBar);
    QPushButton *closeBtn = new QPushButton("\u00D7", m_titleBar);
    closeBtn->setObjectName("closeBtn");

    reduceBtn->setFixedSize(30, 25);
    closeBtn->setFixedSize(30, 25);

    titleLayout->addStretch();
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addWidget(reduceBtn);
    titleLayout->addWidget(closeBtn);

    // =========================================================
    //                    MAIN AREA
    // =========================================================

    QWidget *mainArea = new QWidget(m_mainWidget);
    mainArea->setObjectName("mainArea");
    mainArea->setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *mainAreaLayout = new QVBoxLayout(mainArea);
    mainAreaLayout->setContentsMargins(20, 20, 20, 20);
    mainAreaLayout->setSpacing(15);

    // --- 1. Brightness & Contrast Panel ---
    QHBoxLayout *bcLayout = new QHBoxLayout();
    brightnessSlider = new QSlider(Qt::Horizontal, mainArea);
    brightnessSlider->setRange(-255, 255);
    brightnessSlider->setValue(0);

    contrastSlider = new QSlider(Qt::Horizontal, mainArea);
    contrastSlider->setRange(1, 200);
    contrastSlider->setValue(50);

    bcLayout->addWidget(new QLabel("Brightness:"));
    bcLayout->addWidget(brightnessSlider);
    bcLayout->addSpacing(30);
    bcLayout->addWidget(new QLabel("Contrast:"));
    bcLayout->addWidget(contrastSlider);

    // --- 2. Images & Slice Sliders Panel ---
    QHBoxLayout *imagesLayout = new QHBoxLayout();
    imagesLayout->setSpacing(20);

    auto createViewPanel = [this, mainArea](QLabel *&label, QSlider *&slider,
                                            const QString &titleText) {
        QVBoxLayout *layout = new QVBoxLayout();
        QLabel *title = new QLabel(titleText, mainArea);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet("font-weight: bold;");

        label = new QLabel("(Loading...)", mainArea);
        label->setAlignment(Qt::AlignCenter);
        label->setMinimumSize(256, 256);
        label->setStyleSheet("background-color: #000; border-radius: 5px;");

        slider = new QSlider(Qt::Horizontal, mainArea);

        layout->addWidget(title);
        layout->addWidget(label);
        layout->addWidget(slider);
        return layout;
    };

    imagesLayout->addLayout(createViewPanel(sagittalLabel, sagittalSlider, "Sagittal"));
    imagesLayout->addLayout(createViewPanel(coronalLabel, coronalSlider, "Coronal"));
    imagesLayout->addLayout(createViewPanel(axialLabel, axialSlider, "Axial"));

    mainAreaLayout->addLayout(bcLayout);
    mainAreaLayout->addLayout(imagesLayout);

    // =========================================================
    //                    FINAL ASSEMBLY
    // =========================================================

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(mainArea);

    // =========================================================
    //                     CONNECT
    // =========================================================

    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);

    connect(sagittalSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(coronalSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(axialSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(brightnessSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);
    connect(contrastSlider, &QSlider::valueChanged, this, &NiftiViewerWindow::updateViews);

    // =========================================================
    //                    DATA LOADING
    // =========================================================
    try {
        brainVolume = NiftiVolume::loadNifti(brainPath);
        maskVolume = NiftiVolume::loadNifti(maskPath);
        isLoaded = true;

        std::vector<int64_t> shape = brainVolume.getShape();
        int dimX = shape[0];
        int dimY = shape[1];
        int dimZ = shape[2];

        sagittalSlider->setRange(0, dimX - 1);
        coronalSlider->setRange(0, dimY - 1);
        axialSlider->setRange(0, dimZ - 1);

        sagittalSlider->setValue(dimX / 2);
        coronalSlider->setValue(dimY / 2);
        axialSlider->setValue(dimZ / 2);

    } catch (const std::exception &e) {
        QMessageBox::critical(this, "Error", QString("Failed to load viewer images:\n") + e.what());
        isLoaded = false;
    }
}

NiftiViewerWindow::~NiftiViewerWindow() {}

// =========================================================
//                        METHODS
// =========================================================

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
    return QMainWindow::eventFilter(obj, event);
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
        p.setPen(QPen(Qt::green, 1));
        if (axis == 0) {
            p.drawLine(currentY, 0, currentY, img.height());
            p.drawLine(0, currentZ, img.width(), currentZ);
        } else if (axis == 1) {
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, currentZ, img.width(), currentZ);
        } else if (axis == 2) {
            p.drawLine(currentX, 0, currentX, img.height());
            p.drawLine(0, currentY, img.width(), currentY);
        }
    };

    drawCrosshair(sagImg, 0);
    drawCrosshair(corImg, 1);
    drawCrosshair(axiImg, 2);

    sagittalLabel->setPixmap(QPixmap::fromImage(sagImg).scaled(256, 256, Qt::KeepAspectRatio));
    coronalLabel->setPixmap(QPixmap::fromImage(corImg).scaled(256, 256, Qt::KeepAspectRatio));
    axialLabel->setPixmap(QPixmap::fromImage(axiImg).scaled(256, 256, Qt::KeepAspectRatio));
}

QImage NiftiViewerWindow::extractSlice(int axis, int sliceIndex) {
    std::vector<int64_t> shape = brainVolume.getShape();
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
            float brainVal = 0.0f;
            float maskVal = 0.0f;

            if (axis == 0) {
                brainVal = brainVolume.data(sliceIndex, i, j, 0);
                maskVal = maskVolume.data(sliceIndex, i, j, 0);
            } else if (axis == 1) {
                brainVal = brainVolume.data(i, sliceIndex, j, 0);
                maskVal = maskVolume.data(i, sliceIndex, j, 0);
            } else if (axis == 2) {
                brainVal = brainVolume.data(i, j, sliceIndex, 0);
                maskVal = maskVolume.data(i, j, sliceIndex, 0);
            }

            int pixelVal = std::clamp(static_cast<int>((brainVal * contrast) + brightness), 0, 255);

            if (maskVal > 0.5f) {
                img.setPixel(i, j, qRgb(255, pixelVal / 2, pixelVal / 2));
            } else {
                img.setPixel(i, j, qRgb(pixelVal, pixelVal, pixelVal));
            }
        }
    }

    if (axis == 0)
        return img.mirrored(true, true);
    if (axis == 1)
        return img.mirrored(false, true);
    return img.mirrored(false, true);
}