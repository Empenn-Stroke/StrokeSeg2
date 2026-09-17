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

/**
 * @brief The MainWindow class represents the main window of the application.
 *
 * This class provides the primary user interface for the application, including
 * a parameters form, input file buttons, a stacked widget for different views,
 * and buttons for various actions such as running the pipeline, opening the
 * guide, and managing models. It also handles events for window dragging and
 * provides slots for updating the UI and processing pipeline results.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for MainWindow.
     *
     * Initializes the main window with a specified parent widget and configuration parameters.
     *
     * @param opts The PipelineParams object containing configuration settings.
     * @param parent The parent widget for this window. Defaults to 0.
     */
    explicit MainWindow(const PipelineParams &opts, QWidget *parent = 0);

    /**
     * @brief Destructor for MainWindow.
     */
    ~MainWindow();

  protected:
    /**
     * @brief Reimplemented event filter to handle window dragging.
     *
     * This method overrides the default event filter to allow for window dragging
     * by clicking and dragging the title bar.
     *
     * @param obj The object that received the event.
     * @param event The event that occurred.
     * @return True if the event was handled, false otherwise.
     */
    bool eventFilter(QObject *obj, QEvent *event) override;

    bool m_dragging = false; /**< A boolean indicating whether the window is being dragged. */
    QPoint m_dragPosition; /**< The position where the mouse was clicked for dragging. */

private:
    QWidget *m_mainWidget; /**< The main widget containing the UI elements. */
    QWidget *m_titleBar; /**< The title bar widget for window dragging. */

    ParametersFormWidget *m_parametersForm; /**< The parameters form widget for user input. */

    QWidget *m_inputsContainer; /**< The container widget for input file buttons. */
    QVBoxLayout *m_inputsLayout; /**< The layout for input file buttons. */
    QList<FileDropButton *> m_fileButtons; /**< List of file drop buttons for input files. */
    QMap<QString, QString> m_filesChosen; /**< Map of chosen files and their paths. */

    QStackedWidget *m_stackedArea; /**< The stacked widget for different views. */
    QString *m_fileChosen = nullptr; /**< Pointer to the currently chosen file. */
    QLabel *m_fileLabel = nullptr; /**< Label displaying the currently chosen file. */
    QPushButton *m_runButton; /**< Button to run the pipeline. */
    QLabel *m_consoleLabel; /**< Label for the console window. */

    QPointer<ConsoleWindow> m_consoleWindow = nullptr; /**< Pointer to the console window. */

    QPushButton *m_modelManager; /**< Button to open the model manager. */
    QPushButton *m_resetSettings; /**< Button to reset settings. */
    QPushButton *m_terminalButton; /**< Button to open the terminal. */

    QPointer<GuideWindow> guide; /**< Pointer to the guide window. */
    QPointer<AboutWindow> about; /**< Pointer to the about window. */
    QPointer<ModelManager> modelManager; /**< Pointer to the model manager. */
    WarningWindow *warning = nullptr; /**< Pointer to the warning window. */
    bool showWarning = true; /**< Boolean indicating whether to show the warning window. */

    PipelineParams m_params; /**< PipelineParams object containing configuration settings. */

  private:
    /**
     * @brief Sets up the title bar with buttons for closing and minimizing.
     *
     * @param closeBtn Reference to the close button.
     * @param reduceBtn Reference to the minimize button.
     */
    void titleBar(QPushButton *&closeBtn, QPushButton *&reduceBtn);

    /**
     * @brief Updates the state of the run button.
     */
    void updateRunButtonState();

    /**
     * @brief Sets up input buttons for the specified model.
     *
     * @param inputNames List of input names for the model.
     */
    void setupInputButtonsForModel(const QStringList &inputNames);

private slots:
    /**
     * @brief Enables or disables input buttons.
     *
     * @param enabled Boolean indicating whether to enable the buttons.
     */
    void setInputsEnabled(bool enabled);

    /**
     * @brief Opens the guide window.
     */
    void openGuide();

    /**
     * @brief Opens the about window.
     */
    void openAbout();

    /**
     * @brief Opens the model manager.
     */
    void openModelManager();

    /**
     * @brief Processes the pipeline.
     */
    void Process();

    /**
     * @brief Toggles the console window.
     */
    void toggleConsole();

    /**
     * @brief Handles the close event for the window.
     *
     * @param event The close event.
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Handles the completion of the pipeline.
     *
     * @param success Boolean indicating whether the pipeline completed successfully.
     * @param message Message describing the result of the pipeline.
     * @param finalPath Path to the final output file.
     */
    void onPipelineFinished(bool success, QString message, QString finalPath);

  public slots:
    /**
     * @brief Refreshes the list of models.
     */
    void refreshModelsList();
};

#endif
