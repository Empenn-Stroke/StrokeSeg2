#pragma once

#include "utils/niftiVolume.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QtWidgets/QMainWindow>

class NiftiViewerWindow : public QMainWindow {
    Q_OBJECT

  public:
    NiftiViewerWindow(const QString &brainPath, const QString &maskPath, QWidget *parent = nullptr);
    ~NiftiViewerWindow();

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  private slots:
    void updateViews();

  private:
    // Window Dragging & Custom Title Bar
    QWidget *m_titleBar;
    bool m_dragging = false;
    QPoint m_dragPosition;

    // View Labels
    QLabel *sagittalLabel;
    QLabel *coronalLabel;
    QLabel *axialLabel;

    // View Sliders
    QSlider *sagittalSlider;
    QSlider *coronalSlider;
    QSlider *axialSlider;

    // Adjustments
    QSlider *brightnessSlider;
    QSlider *contrastSlider;

    // Data
    NiftiVolume brainVolume;
    NiftiVolume maskVolume;
    bool isLoaded = false;

    int currentX = 0;
    int currentY = 0;
    int currentZ = 0;

    QImage extractSlice(int axis, int sliceIndex);
};