// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mainWindow.h"

#include <algorithm>
#include <managers/logManager.h>
#include <QProgressBar>

#include <QDebug>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QIcon>
#include <QMessageBox>
#include <QMimeData>
#include <QMovie>
#include <QSettings>
#include <QStandardPaths>
#include <QtConcurrent>
#include <utils/env_path.h>
#include <managers/progressManager.h>
#include <utils/pipelineParams.h>
#include "niftiViewerWindow.h"
#include <widgets/flowLayout.h>

#ifdef Q_OS_WIN
    #include <windows.h>
#endif

/**
 * @brief Constructs a MainWindow object with the given pipeline parameters and parent widget.
 * @param opts The pipeline parameters to be used in the application.
 * @param parent The parent widget.
 */
MainWindow::MainWindow(const PipelineParams &opts, QWidget *parent) : QMainWindow(parent), m_params(opts) 
{

    setWindowTitle("StrokeSeg2");
    setWindowIcon(QIcon(":/gui/resources/StrokeSegPWR.ico"));
    resize(1280, 720);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Warning window
    QSettings settings;
    showWarning = settings.value("showWarning", true).toBool();
    if (showWarning) 
    {
        warning = new WarningWindow();
        warning->exec();
    }

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    m_mainWidget = new QWidget(this);
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

    QHBoxLayout *mainLayout = new QHBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // =========================================================
    //                      TITLE BAR
    // =========================================================

    QPushButton *closeBtn = nullptr;
    QPushButton *reduceBtn = nullptr;

    titleBar(closeBtn, reduceBtn);

    // =========================================================
    //                      MENU BAR
    // =========================================================

    QMenuBar *menuBar = new QMenuBar(m_mainWidget);
    menuBar->setObjectName("menuBar");

    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *actionGuide = new QAction("Guide", this);
    helpMenu->addAction(actionGuide);
    QAction *actionAbout = new QAction("About", this);
    helpMenu->addAction(actionAbout);
    QAction *actionResetWW = new QAction("Show warning", this);
    actionResetWW->setCheckable(true);

    showWarning = settings.value("showWarning", true).toBool();
    actionResetWW->setChecked(showWarning);

    helpMenu->addAction(actionResetWW);

    // =========================================================
    //                     LEFT COLUMN
    // =========================================================

    QWidget *leftColumnBg = new QWidget(m_mainWidget);
    leftColumnBg->setObjectName("leftColumnBg");
    leftColumnBg->setAttribute(Qt::WA_StyledBackground, true);

    QWidget *leftColumn = new QWidget(leftColumnBg);
    leftColumn->setObjectName("leftColumn");
    leftColumn->setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *leftBgLayout = new QVBoxLayout(leftColumnBg);
    leftBgLayout->setContentsMargins(0, 0, 0, 0);
    leftBgLayout->addWidget(leftColumn);

    m_parametersForm = new ParametersFormWidget(leftColumn);

    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    // ---------------- BOTTOM BUTTONS ----------------

    QWidget *bottomBtns = new QWidget(leftColumn);
    bottomBtns->setObjectName("bottomBtns");
    bottomBtns->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBtns);

    m_modelManager = new QPushButton("Model manager");
    m_resetSettings = new QPushButton("Reset settings");
    m_terminalButton = new QPushButton("Show Console");

    bottomLayout->addWidget(m_modelManager);
    bottomLayout->addWidget(m_resetSettings);
    bottomLayout->addWidget(m_terminalButton);

    leftLayout->addSpacing(10);
    leftLayout->addWidget(m_parametersForm, 0, Qt::AlignTop);
    leftLayout->addStretch(1);
    leftLayout->addWidget(bottomBtns);

    // =========================================================
    //                    MAIN AREA
    // =========================================================

    QWidget *mainArea = new QWidget(m_mainWidget);
    mainArea->setObjectName("mainArea");
    mainArea->setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *mainAreaLayout = new QVBoxLayout(mainArea);
    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
    mainAreaLayout->setSpacing(20);

    // Stacked area
    m_stackedArea = new QStackedWidget(mainArea);

    // ---------------- DEFAULT PAGE ----------------
    QWidget *defaultPage = new QWidget();
    QVBoxLayout *defaultLayout = new QVBoxLayout(defaultPage);
    defaultLayout->setContentsMargins(0, 0, 0, 0);
    defaultLayout->setSpacing(0);

    m_inputsContainer = new QWidget(defaultPage);
    m_inputsContainer->setObjectName("inputsContainer");
    m_inputsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_inputsLayout = new QVBoxLayout(m_inputsContainer);
    m_inputsLayout->setContentsMargins(0, 0, 0, 0);
    m_inputsLayout->setSpacing(15);
    m_inputsLayout->setAlignment(Qt::AlignCenter);

    // Run button
    m_runButton = new QPushButton("RUN", defaultPage);
    m_runButton->setObjectName("runBtn");

    // Console log
    QWidget *consoleContainer = new QWidget(defaultPage);
    consoleContainer->setObjectName("consoleContainer");
    consoleContainer->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *consoleLayout = new QHBoxLayout(consoleContainer);
    consoleLayout->setContentsMargins(0, 10, 8, 0);
    consoleLayout->setSpacing(0);
    consoleLayout->setAlignment(Qt::AlignRight);

    m_consoleLabel = new QLabel(mainArea);
    m_consoleLabel->setObjectName("consoleLabel");
    m_consoleLabel->setText("v1.0.0");
    consoleLayout->addWidget(m_consoleLabel);

    // Default area assembly
    defaultLayout->addWidget(m_inputsContainer);

    defaultLayout->addStretch(1);
    defaultLayout->addWidget(m_runButton, 0, Qt::AlignHCenter);
    defaultLayout->addStretch(4);
    defaultLayout->addWidget(consoleContainer, 0, Qt::AlignRight);

    // ---------------- LOADING PAGE ----------------

    QWidget *loadingPage = new QWidget();
    QVBoxLayout *loadingLayout = new QVBoxLayout(loadingPage);
    QWidget *barContainer = new QWidget(loadingPage);
    QHBoxLayout *barLayout = new QHBoxLayout(barContainer);
    barLayout->setContentsMargins(0, 0, 0, 0);
    barLayout->setSpacing(10);

    QLabel *spinnerLabel = new QLabel(loadingPage);
    QMovie *movie = new QMovie(":/gui/resources/infinite-spinner-optimized.gif");
    spinnerLabel->setMovie(movie);
    spinnerLabel->setAlignment(Qt::AlignCenter);
    movie->start();

    QProgressBar *progressBar = new QProgressBar(barContainer);
    progressBar->setObjectName("progressBar");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFixedSize(350, 3);
    progressBar->setTextVisible(false);

    QLabel *percentLabel = new QLabel("0%", barContainer);
    percentLabel->setObjectName("percentLabel");
    percentLabel->setFixedWidth(45);
    percentLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    barLayout->addSpacing(45);
    barLayout->addWidget(progressBar);
    barLayout->addWidget(percentLabel);

    barContainer->setFixedSize(barLayout->sizeHint().width(), 20);

    QLabel *statusLabel = new QLabel("Initializing...", loadingPage);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setFixedWidth(400);
    statusLabel->setAlignment(Qt::AlignCenter);

    QPushButton *cancelBtn = new QPushButton("Cancel", loadingPage);
    cancelBtn->setObjectName("cancelBtn");
    cancelBtn->setFixedWidth(120);
    cancelBtn->setCursor(Qt::PointingHandCursor);

    loadingLayout->addStretch(2);
    loadingLayout->addWidget(spinnerLabel, 0, Qt::AlignCenter);
    loadingLayout->addSpacing(30);
    loadingLayout->addWidget(barContainer, 0, Qt::AlignCenter);
    loadingLayout->addSpacing(10);
    loadingLayout->addWidget(statusLabel, 0, Qt::AlignCenter);
    loadingLayout->addSpacing(20);
    loadingLayout->addWidget(cancelBtn, 0, Qt::AlignCenter);
    loadingLayout->addStretch(3);

    // ---------------- MAIN AREA ASSEMBLY ----------------

    m_stackedArea->addWidget(defaultPage);
    m_stackedArea->addWidget(loadingPage);
    mainAreaLayout->addWidget(m_stackedArea);

    // =========================================================
    //                    FINAL ASSEMBLY
    // =========================================================

    mainLayout->addWidget(leftColumnBg);
    mainLayout->addWidget(mainArea);
    mainLayout->setStretch(0, 3);
    mainLayout->setStretch(1, 7);

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(menuBar);
    windowLayout->addLayout(mainLayout);

    // =========================================================
    //                     CONNECT
    // =========================================================

    connect(m_modelManager, &QPushButton::clicked, this, &MainWindow::openModelManager);
    connect(m_terminalButton, &QPushButton::clicked, this, &MainWindow::toggleConsole);
    connect(m_resetSettings, &QPushButton::clicked, m_parametersForm, &ParametersFormWidget::resetFields);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::Process);

    connect(actionGuide, &QAction::triggered, this, &MainWindow::openGuide);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::openAbout);

    connect(actionResetWW, &QAction::toggled, this, [this](bool checked) 
    {
        showWarning = checked;
        QSettings settings;
        settings.setValue("showWarning", showWarning);
    });

    connect(&ProgressManager::instance(), &ProgressManager::progressUpdated, this,
            [progressBar, percentLabel](int value)
    {
        progressBar->setValue(value);
        percentLabel->setText(QString::number(value) + "%");
    });

    connect(&ProgressManager::instance(), &ProgressManager::progressStatusChanged, statusLabel,
            &QLabel::setText);

    connect(cancelBtn, &QPushButton::clicked, this,
            [this]() { ProgressManager::instance().requestInterruption(); });

    connect(m_parametersForm, &ParametersFormWidget::parametersChanged, this, &MainWindow::updateRunButtonState);

    connect(m_parametersForm, &ParametersFormWidget::modelChanged, this, &MainWindow::setupInputButtonsForModel);

    QStringList defaultInputs = m_parametersForm->getCurrentModelInputs();
    setupInputButtonsForModel(defaultInputs);

    updateRunButtonState();

    // =========================================================
    // 				   LOAD SETTINGS
    // =========================================================

    if (m_params.inputPaths.isEmpty() && m_params.outputDir.isEmpty() && !m_params.gui)
    {
        m_parametersForm->loadSettings();
    } 
    else
    {
        m_parametersForm->setParams(m_params);
    }

    if (!m_params.inputPaths.isEmpty())
    {
        m_filesChosen = m_params.inputPaths;
    }
}

void MainWindow::titleBar(QPushButton *&closeBtn, QPushButton *&reduceBtn) 
{
    m_titleBar = new QWidget(m_mainWidget);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(25);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    closeBtn = new QPushButton("\u00D7", m_titleBar);
    reduceBtn = new QPushButton("\u2212", m_titleBar);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(60, 0, 0, 2);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("StrokeSeg2", m_titleBar);
    closeBtn->setObjectName("closeBtn");

    reduceBtn->setFixedSize(30, 25);
    closeBtn->setFixedSize(30, 25);

    titleLayout->addStretch();
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addWidget(reduceBtn);
    titleLayout->addWidget(closeBtn);
}

// =========================================================
//                        METHODS
// =========================================================

/**
 * @brief Event filter to handle mouse events for dragging the window when clicking and dragging the title bar.
 * @param obj The object that received the event.
 * @param event The event that occurred.
 * @return True if the event was handled, false otherwise.
 */
bool MainWindow::eventFilter(QObject *obj, QEvent *event) 
{
    bool result = QMainWindow::eventFilter(obj, event);

    if (obj == m_titleBar) 
    {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease) 
        {
            auto *e = static_cast<QMouseEvent *>(event);
            if (event->type() == QEvent::MouseButtonPress && e->button() == Qt::LeftButton) 
            {
                m_dragging = true;
                m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
                result = true;
            }
            if (event->type() == QEvent::MouseMove && m_dragging) 
            {
                move(e->globalPosition().toPoint() - m_dragPosition);
                result = true;
            }
            if (event->type() == QEvent::MouseButtonRelease) 
            {
                m_dragging = false;
                result = true;
            }
        }
    }
    return result;
}

/**
 * @brief Opens the guide window. If the guide window does not exist, it creates a new instance.
 */
void MainWindow::openGuide() 
{
    if (guide.isNull()) 
    {
        guide = new GuideWindow();
    }

    guide->show();
    guide->raise();
    guide->activateWindow();
}

/**
 * @brief Opens the about window. If the about window does not exist, it creates a new instance.
 */
void MainWindow::openAbout() 
{
    if (about.isNull())
    {
        about = new AboutWindow();
    }

    about->show();
    about->raise();
    about->activateWindow();
}

/**
 * @brief Opens the model manager window. If the model manager window does not exist, it creates a new instance and connects its signals to refresh the models list in the main window.
 */
void MainWindow::openModelManager() 
{
    qDebug() << "openModelManager called, modelManager isNull:" << modelManager.isNull();

    if (modelManager.isNull()) 
    {
        modelManager = new ModelManager(this);
        connect(modelManager, &ModelManager::modelsChanged, this, &MainWindow::refreshModelsList);
    }

    modelManager->show();
    modelManager->raise();
    modelManager->activateWindow();

    int targetX = this->x() + modelManager->width() + 32;
    int targetY = this->y() + this->height() - modelManager->height() - 15;

    QScreen *screen = this->screen();
    if (screen)
    {
        QRect available = screen->availableGeometry();

        targetX = qBound(available.left(), targetX, available.right() - modelManager->width());
        targetY = qBound(available.top(), targetY, available.bottom() - modelManager->height());
    }

    modelManager->move(targetX, targetY);

    qDebug() << "ModelManager geometry:" << modelManager->geometry() << "visible:" << modelManager->isVisible();
}

/**
 * @brief Refreshes the list of available models in the parameters form by reading the model directory and updating the UI accordingly.
 */
void MainWindow::refreshModelsList() 
{
    QDir modelsDir(Paths::modelDir());
    QStringList entries = modelsDir.entryList(QStringList() << "*.onnx", QDir::Files | QDir::NoDotAndDotDot);
    m_parametersForm->refreshModels(entries);
}

/**
 * @brief Enables or disables the input fields and buttons in the main window based on the provided boolean value.
 * @param enabled True to enable the inputs, false to disable them.
 */
void MainWindow::setInputsEnabled(bool enabled) 
{
    m_parametersForm->setInputsEnabled(enabled);
    m_runButton->setEnabled(enabled);

    for (FileDropButton *btn : m_fileButtons)
    {
        if (btn != nullptr)
        {
            btn->setEnabled(enabled);
        }
    }
}

/**
 * @brief Processes the selected file using the pipeline with the specified parameters. It checks for valid input and output paths, creates the output directory, and starts the pipeline in a separate thread. The UI is updated to show progress and
 * status messages during processing.
 */
void MainWindow::Process() 
{
    m_params = m_parametersForm->getParams();
    m_params.inputPaths = m_filesChosen;
    m_params.inputOrder = m_parametersForm->getCurrentModelInputs();
    m_params.modalities = m_parametersForm->getCurrentModelModalities();

    bool canProceed = true;
    QString referencePath;

    if (!m_params.inputOrder.isEmpty() && m_filesChosen.contains(m_params.inputOrder.first()))
    {
        referencePath = m_filesChosen.value(m_params.inputOrder.first());
    }

    if (referencePath.isEmpty()) 
    {
        m_consoleLabel->setText("Error: No input files provided.");
        canProceed = false;
    }

    if (canProceed) 
    {
        QString modelName = QFileInfo(m_params.modelPath).baseName();
        QString subjectName = QFileInfo(referencePath).baseName();
        m_params.outputDir = m_params.outputDir + "/" + subjectName + "_" + modelName;

        if (!QDir().mkpath(m_params.outputDir)) 
        {
            m_consoleLabel->setText("Failed to create output directory.");
            canProceed = false;
        }
        else
        {
            QSettings settings;
            QStringList recentDirs = settings.value("recentOutputDirs").toStringList();
            recentDirs.removeAll(m_params.outputDir);
            recentDirs.prepend(m_params.outputDir);

            while (recentDirs.size() > 20) 
            {
                recentDirs.removeLast();
            }

            settings.setValue("recentOutputDirs", recentDirs);
        }

    }

    if (canProceed)
    {
        setInputsEnabled(false);
        m_stackedArea->setCurrentIndex(1);
        m_consoleLabel->setText("Starting pipeline...");

        QThread *thread = new QThread;
        PipelineWorker *worker = new PipelineWorker(m_params);
        worker->moveToThread(thread);

        connect(thread, &QThread::started, worker, &PipelineWorker::process);
        connect(worker, &PipelineWorker::statusChanged, this, [this](QString msg) { m_consoleLabel->setText(msg); });
        connect(worker, &PipelineWorker::finished, this, &MainWindow::onPipelineFinished);
        connect(worker, &QObject::destroyed, thread, &QThread::quit);
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);

        thread->start();
    }
}


/**
 * @brief Slot that is called when the pipeline processing is finished. It updates the UI based on the success or failure of the pipeline, opens the output folder or viewer if specified, and cleans up the worker thread.
 * @param success Indicates whether the pipeline processing was successful.
 * @param message A message to display in the console label, typically indicating the result of the pipeline processing.
 * @param finalPath The path to the final output file generated by the pipeline, if applicable.
 */
void MainWindow::onPipelineFinished(bool success, QString message, QString finalPath) 
{
    setInputsEnabled(true);
    m_stackedArea->setCurrentIndex(0);
    m_consoleLabel->setText(message);

    sender()->deleteLater();

    if (success) 
    {
        if (m_params.openFolder) 
        {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_params.outputDir));
        }

        if (m_params.openViewer && !finalPath.isEmpty()) 
        {
            QString baseImagePath;

            QString referenceInput = m_params.inputOrder.isEmpty() ? "" : m_params.inputPaths.value(m_params.inputOrder.first());

            if (m_params.betOnly) 
            {
                finalPath = "";
                baseImagePath = m_params.outputDir + "/" + QFileInfo(referenceInput).baseName() + "_BET.nii.gz";
            }

            if (m_params.mni) 
            {
                baseImagePath = m_params.outputDir + "/" + "MNI_" + QFileInfo(referenceInput).baseName() + "_BET.nii.gz";
            } 
            else 
            {
                baseImagePath = referenceInput;
            }


            NiftiViewerWindow *viewer = new NiftiViewerWindow(baseImagePath, finalPath, this);
            qDebug() << "Opening viewer with MRI:" << baseImagePath << "and mask:" << finalPath;

            // Qt will automatically delete the window from memory when the user closes it
            viewer->setAttribute(Qt::WA_DeleteOnClose);

            // Force it to open as an independent window, not embedded inside the main UI
            viewer->setWindowFlag(Qt::Window);
            viewer->show();
        }
    } 
    else 
    {
        QMessageBox::critical(this, "Pipeline Error", message);
    }
}

/**
 * @brief Toggles the visibility of the console window on Windows. If the console is not currently allocated, it allocates a new console and redirects stdout and stderr to it. It also updates the text of the terminal button to reflect the current
 * state (showing or hiding the console).
 */
void MainWindow::toggleConsole() 
{
    if (m_consoleWindow.isNull()) 
    {
        m_consoleWindow = new ConsoleWindow(this);
        m_consoleWindow->setAttribute(Qt::WA_DeleteOnClose);
    }

    if (m_consoleWindow->isVisible()) 
    {
        m_consoleWindow->hide();
        m_terminalButton->setText("Show Console");
    } 
    else 
    {
        m_consoleWindow->show();
        m_consoleWindow->raise();
        m_consoleWindow->activateWindow();
        m_terminalButton->setText("Hide Console");
    }
}

/**
 * @brief Handles the close event for the main window. It ensures that any open model manager, guide, or about windows are closed, and saves the settings of the parameters form if the application is not running in GUI mode.
 * @param event The close event that triggered this function.
 */
void MainWindow::closeEvent(QCloseEvent *event) 
{
    if (!modelManager.isNull()) 
    {
        modelManager->close();
    }

    if (!guide.isNull()) 
    {
        guide->close();
    }

    if (!about.isNull()) 
    {
        about->close();
    }

    if (!m_params.gui) 
    {
        m_parametersForm->saveSettings();
    }
    event->accept();
}

/**
 * @brief Updates the state of the "Run" button based on the current parameters and file selections. The button is enabled only if all required files are provided and the parameters are valid.
 */
void MainWindow::updateRunButtonState() {
    PipelineParams currentParams = m_parametersForm->getParams();
    QStringList requiredInputs = m_parametersForm->getCurrentModelInputs();

    currentParams.inputPaths = m_filesChosen;

    QString validationError;
    bool isValid = currentParams.isValid(requiredInputs, validationError);

    m_runButton->setEnabled(isValid);

    if (isValid) 
    {
        m_consoleLabel->setText("Ready");
    }
    else
    {
        m_consoleLabel->setText(validationError);
    }
}

/**
 * @brief Sets up the input buttons for the model based on the provided list of input names. It clears any existing buttons and creates new buttons for each input name, allowing users to drop or choose files for each input. The function also connects
 * the buttons to
 * @param inputNames A list of input names for which buttons will be created. Each button will allow users to drop or choose a file corresponding to the input name.
 */
void MainWindow::setupInputButtonsForModel(const QStringList &inputNames) 
{

    qDebug() << "[UI] Nombre d'entrées reçues par la fonction UI :" << inputNames.size();
    qDebug() << "[UI] Liste :" << inputNames;

    qDeleteAll(m_fileButtons);
    m_fileButtons.clear();
    m_filesChosen.clear();

    if (m_inputsContainer->layout()) 
    {
        delete m_inputsContainer->layout();
    }

    FlowLayout *inputsLayout = new FlowLayout(m_inputsContainer, 24, 20, 20);
    inputsLayout->setMaxGrowthFactor(1.3);

    for (int i = 0; i < inputNames.size(); ++i)
    {
        QString rawInputName = inputNames[i];
        QString inputName = inputNames[i].toUpper(); 

        FileDropButton *btn = new FileDropButton(m_inputsContainer);
        btn->setObjectName(QString("chooseFileButton_%1").arg(inputName.toLower()));
        btn->setAcceptDrops(true);
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        btn->setMinimumSize(220, 170);

        QVBoxLayout *buttonLayout = new QVBoxLayout(btn);
        buttonLayout->setContentsMargins(8, 8, 8, 8);
        buttonLayout->setSpacing(4);
        buttonLayout->addStretch();

        QLabel *iconLabel = new QLabel(btn);
        QPixmap pix(":/gui/resources/files.png");
        iconLabel->setPixmap(pix.scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        buttonLayout->addWidget(iconLabel);

        QLabel *fileLabel = new QLabel(QString("Choose %1 file").arg(inputName), btn);
        fileLabel->setObjectName(QString("fileLabel_%1").arg(inputName.toLower()));
        fileLabel->setAlignment(Qt::AlignCenter);
        fileLabel->setWordWrap(true);
        fileLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        buttonLayout->addWidget(fileLabel);
        buttonLayout->addStretch();

        auto handleFileSelection = [this, rawInputName, fileLabel](const QString &filePath) {
            if (!filePath.isEmpty()) {
                m_filesChosen[rawInputName] = filePath;
                fileLabel->setText(QFileInfo(filePath).fileName());
                updateRunButtonState();
            }
        };

        connect(btn, &QToolButton::clicked, this, [this, inputName, handleFileSelection]()
        {
            QSettings settings;
            QString lastDirFile = settings.value("lastInputPath", QDir::homePath()).toString();
            QString filters = "Medical Images (*.nii *.nii.gz *.nrrd *.dcm);;All files (*)";
            QString filePath = QFileDialog::getOpenFileName(this, QString("Choose %1 file").arg(inputName), lastDirFile, filters);

            if (!filePath.isEmpty())
            {
                QSettings().setValue("lastInputPath", QFileInfo(filePath).absolutePath());
                handleFileSelection(filePath);
            }
        });

        connect(btn, &FileDropButton::fileDropped, this, handleFileSelection);
        connect(btn, &FileDropButton::errorOccurred, this, [this](const QString &error) { m_consoleLabel->setText(error); });

        inputsLayout->addWidget(btn);
        m_fileButtons.append(btn);
    }

    inputsLayout->activate();

    updateRunButtonState();
}

/**
 * @brief Destructor for the MainWindow class. It saves the settings of the parameters form if the application is not running in GUI mode.
 */
MainWindow::~MainWindow() 
{
    if (!m_params.gui) 
    {
        m_parametersForm->saveSettings();
    }
}
