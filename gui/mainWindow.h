#pragma once
#ifndef MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsDropShadowEffect>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

#include <QMenuBar>

#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QSlider>
#include <QComboBox>
#include <QDoubleValidator>
#include <QFileDialog>
#include <QFormLayout>
#include <QIcon>
#include <algorithm>

#include <QSettings>

#include "guideWindow.h"
#include "warningWindow.h"

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    MainWindow(QWidget *parent=0);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    bool m_dragging = false;
    QPoint m_dragPosition;

private:
    QWidget *m_mainWidget;
    QWidget *m_titleBar;
    QFormLayout *m_formLayout;

    QLineEdit *m_suffix;
    QComboBox *m_model;
    QCheckBox *m_toggleView;
    QCheckBox *m_toggleOutput;
    QCheckBox *m_skipBrainExtract;
    QCheckBox *m_savePMap;
    QCheckBox *m_savePreprocessing;
    QComboBox *m_mode;
    QSlider *m_thresholdSlider;
    QLineEdit *m_threshold;

    QToolButton *m_fileButton;
    QPushButton *m_runButton;

    QPushButton *m_importModel;


    GuideWindow *guide = nullptr;
    WarningWindow *warning = nullptr;
    bool showWarning = true;


  private slots:
    void chooseFile();
    void openGuide();
    void importModel();
    double sliderValueToReal(int sliderValue);
    int realToSliderValue(double realValue);
    QString formatThreshold(double v);
};


#endif // MAINWINDOW_H

