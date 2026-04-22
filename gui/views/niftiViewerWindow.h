#pragma once

#include "ClickableLabel.h"
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

class NiftiViewerWindow : public QDialog {
    Q_OBJECT

  public:
    // Updated constructor to accept the base and mask paths
    explicit NiftiViewerWindow(const QString &t1Path, const QString &finalPath,
                               QWidget *parent = nullptr);
    ~NiftiViewerWindow();

    // Updated loadVolume to accept an optional mask path
    void loadVolume(const QString &fileName, const QString &maskFileName = "");

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

  private slots:
    void handleImageClick(int axis, int px, int py, ClickableLabel *label);
    void openNiftiFile();
    void updateViews();

  private:
    // --- Window Dragging State ---
    QWidget *m_titleBar;
    bool m_dragging = false;
    QPoint m_dragPosition;

    // --- File Paths ---
    QString m_t1Path;
    QString m_finalPath;

    // --- UI Elements ---
    QPushButton *openButton;
    QLabel *statusLabel;

    ClickableLabel *sagittalLabel;
    ClickableLabel *coronalLabel;
    ClickableLabel *axialLabel;

    QSlider *sagittalSlider;
    QSlider *coronalSlider;
    QSlider *axialSlider;

    QSlider *brightnessSlider;
    QSlider *contrastSlider;

    // --- State Variables ---
    NiftiVolume myVolume;
    bool isLoaded = false;

    // --- Mask Variables ---
    NiftiVolume myMaskVolume;
    bool isMaskLoaded = false;

    int currentX = 0;
    int currentY = 0;
    int currentZ = 0;

    QImage extractSlice(int axis, int sliceIndex);
};