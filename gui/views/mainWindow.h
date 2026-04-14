#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#include <QStackedWidget>
#include <QPointer>
#include <QTimer>

#include <managers/configmanager.h>
#include "guideWindow.h"
#include "aboutWindow.h"
#include "models/modelManager.h"
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
    QLineEdit *m_destination; QPushButton *m_destinationButton;
    QComboBox *m_model;
    QCheckBox *m_toggleView;
    QCheckBox *m_toggleOpenFolder;
    QCheckBox *m_toggleOutput;
    QCheckBox *m_skipBrainExtract;
    QCheckBox *m_savePMap;
    QCheckBox *m_savePreprocessing;
    QComboBox *m_mode;
    QSlider *m_thresholdSlider; QLineEdit *m_threshold;

    QStackedWidget *m_stackedArea;
    QToolButton *m_fileButton;
    QString *m_fileChosen = nullptr;
    QLabel *m_fileLabel = nullptr;
    QPushButton *m_runButton;
    QLabel *m_consoleLabel;

    QPushButton *m_modelManager;
    QPushButton *m_resetSettings;

    QPointer<GuideWindow> guide;
    QPointer<AboutWindow> about;
    QPointer<ModelManager> modelManager;
    WarningWindow *warning = nullptr;
    bool showWarning = true;


  private slots:
    void chooseDestination();
    void chooseFile();

    double sliderValueToReal(int sliderValue);
    int realToSliderValue(double realValue);
    QString formatThreshold(double v);

    void setInputsEnabled(bool enabled);

    void openGuide();
    void openAbout();
    void openModelManager();

    void Process();

    void saveSettings();
    void loadSettings();

    void closeEvent(QCloseEvent *event) override;

    void onPipelineFinished(bool success, QString message, QString finalPath);

  public slots:
    void refreshModelsList();
};


#endif // MAINWINDOW_H

