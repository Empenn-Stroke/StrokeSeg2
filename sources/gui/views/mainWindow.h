// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "aboutWindow.h"
#include "guideWindow.h"
#include "modelManager.h"
#include "warningWindow.h"
#include <managers/configmanager.h>
#include <widgets/fileDropButton.h>
#include <widgets/parametersFormWidget.h>
#include <workers/pipelineWorker.h>
#include <views/consoleWindow.h>


class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(const PipelineParams &opts, QWidget *parent = 0);
    ~MainWindow();

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    bool m_dragging = false;
    QPoint m_dragPosition;

  private:
    QWidget *m_mainWidget;
    QWidget *m_titleBar;

    ParametersFormWidget *m_parametersForm;

    QWidget *m_inputsContainer;
    QVBoxLayout *m_inputsLayout;
    QList<FileDropButton *> m_fileButtons;
    QMap<QString, QString> m_filesChosen;

    QStackedWidget *m_stackedArea;
    QString *m_fileChosen = nullptr;
    QLabel *m_fileLabel = nullptr;
    QPushButton *m_runButton;
    QLabel *m_consoleLabel;

    QPointer<ConsoleWindow> m_consoleWindow = nullptr;

    QPushButton *m_modelManager;
    QPushButton *m_resetSettings;
    QPushButton *m_terminalButton;

    QPointer<GuideWindow> guide;
    QPointer<AboutWindow> about;
    QPointer<ModelManager> modelManager;
    WarningWindow *warning = nullptr;
    bool showWarning = true;

    PipelineParams m_params;

  private:
    void titleBar(QPushButton *&closeBtn, QPushButton *&reduceBtn);
    void updateRunButtonState();

    void setupInputButtonsForModel(const QStringList &inputNames);


  private slots:

    void setInputsEnabled(bool enabled);

    void openGuide();
    void openAbout();
    void openModelManager();

    void Process();

    void toggleConsole();
    void closeEvent(QCloseEvent *event) override;
    void onPipelineFinished(bool success, QString message, QString finalPath);

  public slots:
    void refreshModelsList();
};

#endif
