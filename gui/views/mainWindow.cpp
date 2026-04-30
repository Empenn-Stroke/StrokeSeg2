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
#include "niftiViewerWindow.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainWindow::MainWindow(const PipelineParams &opts, QWidget *parent)
    : QMainWindow(parent), m_params(opts) {

    setWindowTitle("StrokeSeg2");
    setWindowIcon(QIcon(":/gui/resources/StrokeSeg2.ico"));
    resize(1280, 720);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QSettings::setDefaultFormat(QSettings::IniFormat);

    // Warning window
    QSettings settings;
    showWarning = settings.value("showWarning", true).toBool();
    if (showWarning) {
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

    m_titleBar = new QWidget(m_mainWidget);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(25);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(60, 0, 0, 2);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("StrokeSeg2", m_titleBar);
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

    QVBoxLayout *leftLayout = new QVBoxLayout(leftColumn);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    // ---------------- FORM ----------------

    QWidget *formParameters = new QWidget(leftColumn);
    formParameters->setObjectName("formParameters");
    formParameters->setAttribute(Qt::WA_StyledBackground, true);

    m_formLayout = new QFormLayout(formParameters);
    m_formLayout->setContentsMargins(0, 0, 0, 0);
    m_formLayout->setSpacing(8);

    // Suffix
    m_suffix = new QLineEdit(formParameters);
    m_suffix->setObjectName("suffix");
    m_suffix->setPlaceholderText("Enter the suffix name");

    // Destination

    QWidget *destinationContainer = new QWidget(formParameters);
    destinationContainer->setObjectName("destinationContainer");
    destinationContainer->setContentsMargins(0, 0, 0, 0);
    destinationContainer->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *destinationLayout = new QHBoxLayout(destinationContainer);
    destinationLayout->setSpacing(8);
    destinationLayout->setContentsMargins(0, 0, 0, 0);
    destinationLayout->setObjectName("destinationLayout");

    m_destination = new QLineEdit(formParameters);
    m_destination->setObjectName("destination");
    m_destination->setPlaceholderText("Select output folder");

    m_destinationButton = new QPushButton(formParameters);
    m_destinationButton->setText("...");
    m_destinationButton->setObjectName("destinationBtn");

    destinationLayout->addWidget(m_destination);
    destinationLayout->addWidget(m_destinationButton);

    // Model
    m_model = new QComboBox(formParameters);
    QDir modelsDir = Paths::modelDir();
    QStringList entries = modelsDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &entry : entries) {
        QFileInfo info(modelsDir.filePath(entry));
        QString displayName = info.baseName();
        m_model->addItem(displayName);
    }

    // Toggle
    m_toggleView = new QCheckBox("", formParameters);
    m_toggleOpenFolder = new QCheckBox("", formParameters);
    m_toggleOutput = new QCheckBox("", formParameters);
    m_skipPreProcessing = new QCheckBox("", formParameters);
    m_skipInference = new QCheckBox("", formParameters);
    m_skipPostProcessing = new QCheckBox("", formParameters);
    m_savePMap = new QCheckBox("", formParameters);
    m_savePreProcessing = new QCheckBox("", formParameters);

    // Prediction mode
    m_mode = new QComboBox(formParameters);
    m_mode->addItems({"Prediction", "Brain Extraction Only"});

    // Threshold
    m_thresholdSlider = new QSlider(Qt::Horizontal, formParameters);
    m_thresholdSlider->setRange(0, 100);
    m_thresholdSlider->setValue(50);
    m_thresholdSlider->setFixedWidth(146);

    m_threshold = new QLineEdit("0.50", formParameters);
    m_threshold->setObjectName("thresholdLine");

    auto *validator = new QDoubleValidator(0.0, 1.0, 2, this);
    validator->setLocale(QLocale::C);
    m_threshold->setValidator(validator);

    m_thresholdContainer = new QWidget(formParameters);
    m_thresholdContainer->setObjectName("thresholdContainer");
    m_thresholdContainer->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *thresholdLayout = new QHBoxLayout(m_thresholdContainer);
    thresholdLayout->setContentsMargins(0, 0, 0, 0);
    thresholdLayout->addWidget(m_threshold);
    thresholdLayout->addWidget(m_thresholdSlider);
    thresholdLayout->setSpacing(8);

    // Assembly
    m_formLayout->addRow("Suffix :", m_suffix);

    QLabel *destinationLabel = new QLabel("Destination :", formParameters);
    m_formLayout->addRow(destinationLabel, destinationContainer);

    m_formLayout->addRow("Model :", m_model);
    m_formLayout->addRow("Open viewer :", m_toggleView);
    m_formLayout->addRow("Open destination folder :", m_toggleOpenFolder);
    m_formLayout->addRow("Output MNI space :", m_toggleOutput);
    m_formLayout->addRow("Skip pre-processing:", m_skipPreProcessing);
    m_formLayout->addRow("Skip inference:", m_skipInference);
    m_formLayout->addRow("Skip post-processing:", m_skipPostProcessing);
    m_formLayout->addRow("Save probability map :", m_savePMap);
    m_formLayout->addRow("Save pre-processing :", m_savePreProcessing);
    m_formLayout->addRow("Execution mode :", m_mode);

    QLabel *thresholdLabel = new QLabel("Threshold :", formParameters);
    m_formLayout->addRow(thresholdLabel, m_thresholdContainer);

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
    leftLayout->addWidget(formParameters, 0, Qt::AlignTop);
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

    m_fileButton = new QToolButton(defaultPage);
    m_fileButton->setObjectName("chooseFileButton");
    m_fileButton->setAcceptDrops(true);
    m_fileButton->installEventFilter(this);
    m_fileButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Icon and label layout
    QVBoxLayout *buttonLayout = new QVBoxLayout(m_fileButton);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(20);
    buttonLayout->addStretch();

    // Icon
    QLabel *iconLabel = new QLabel(m_fileButton);
    QPixmap pix(":/gui/resources/files.png");
    iconLabel->setPixmap(pix.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    buttonLayout->addWidget(iconLabel);

    // Text
    m_fileLabel = new QLabel("Choose file", m_fileButton);
    m_fileLabel->setObjectName("fileLabel");
    m_fileLabel->setAlignment(Qt::AlignCenter);
    m_fileLabel->setWordWrap(true);
    m_fileLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    buttonLayout->addWidget(m_fileLabel);

    buttonLayout->addStretch();

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
    defaultLayout->addStretch(3);
    defaultLayout->addWidget(m_fileButton, 0, Qt::AlignHCenter);
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

    connect(m_fileButton, &QToolButton::clicked, this, &MainWindow::chooseFile);
    connect(m_destinationButton, &QPushButton::clicked, this, &MainWindow::chooseDestination);
    connect(m_modelManager, &QPushButton::clicked, this, &MainWindow::openModelManager);
    connect(m_terminalButton, &QPushButton::clicked, this, &MainWindow::toggleConsole);
    connect(m_resetSettings, &QPushButton::clicked, this, [this]() {
        m_suffix->setText("");
        m_destination->setText("");
        m_toggleView->setChecked(false);
        m_toggleOpenFolder->setChecked(false);
        m_toggleOutput->setChecked(false);
        m_savePMap->setChecked(false);
        m_savePreProcessing->setChecked(false);
        m_threshold->setText("0.50");
        m_thresholdSlider->setValue(50);
        m_model->setCurrentIndex(1);
        m_mode->setCurrentIndex(0);
        m_skipPreProcessing->setChecked(false);
        m_skipInference->setChecked(false);
        m_skipPostProcessing->setChecked(false);

        m_consoleLabel->setText("Settings reset");
    });
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::Process);

    connect(m_thresholdSlider, &QSlider::valueChanged, this, [this](int v) {
        double realVal = sliderValueToReal(v);
        m_threshold->blockSignals(true);
        m_threshold->setText(formatThreshold(realVal));
        m_threshold->blockSignals(false);
    });

    connect(m_threshold, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool ok;
        double val = 0.0;

        if (text == "1-10\u207B\u2075") {
            val = 1.0 - 1e-5;
        }
        else if (text == "10\u207B\u2075") {
            val = 1e-5;
        }
        else if (text == "1-10\u207B\u2074") {
            val = 1.0 - 1e-4;
        }
        else if (text == "10\u207B\u2074") {
            val = 1e-4;
        }
        else {
            val = text.toDouble(&ok);
            if (!ok) {
                return;
            }
        }

        int sliderVal = realToSliderValue(val);

        if (sliderVal != m_thresholdSlider->value()) {
            m_thresholdSlider->blockSignals(true);
            m_thresholdSlider->setValue(sliderVal);
            m_thresholdSlider->blockSignals(false);
        }
    });

    connect(actionGuide, &QAction::triggered, this, &MainWindow::openGuide);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::openAbout);

    connect(actionResetWW, &QAction::toggled, this, [this](bool checked) {
        showWarning = checked;
        QSettings settings;
        settings.setValue("showWarning", showWarning);
    });

    connect(m_mode, &QComboBox::currentIndexChanged, this, [this, thresholdLabel](int index) {
        bool isBetOnly = (index == 1);

        m_formLayout->setRowVisible(thresholdLabel, !isBetOnly);
        m_thresholdContainer->setVisible(
            !isBetOnly);

        m_skipPreProcessing->setChecked(false);
        m_skipPreProcessing->setEnabled(!isBetOnly);

        m_skipInference->setChecked(false);
        m_skipInference->setEnabled(!isBetOnly);

        m_skipPostProcessing->setChecked(false);
        m_skipPostProcessing->setEnabled(!isBetOnly);

        m_savePMap->setChecked(false);
        m_savePMap->setEnabled(!isBetOnly);

        m_savePreProcessing->setChecked(false);
        m_savePreProcessing->setEnabled(!isBetOnly);
    });

    connect(&ProgressManager::instance(), &ProgressManager::progressUpdated, this,
            [progressBar, percentLabel](int value) {
                progressBar->setValue(value);
                percentLabel->setText(QString::number(value) + "%");
            });

    connect(&ProgressManager::instance(), &ProgressManager::progressStatusChanged, statusLabel,
            &QLabel::setText);

    connect(cancelBtn, &QPushButton::clicked, this,
            [this]() { ProgressManager::instance().requestInterruption(); });

    // =========================================================
    // 				   LOAD SETTINGS
    // =========================================================

    if (m_params.t1Path.isEmpty() && m_params.outputDir.isEmpty() && !m_params.gui) {
        loadSettings();
    }

    if (!m_params.t1Path.isEmpty()) {
        m_fileLabel->setText(QFileInfo(m_params.t1Path).fileName());
        m_fileChosen = new QString(m_params.t1Path);
    }

    if (!m_params.outputDir.isEmpty()) {
        m_destination->setText(m_params.outputDir);
        m_destination->setCursorPosition(m_destination->text().length());
    }

    if (!m_params.suffix.isEmpty()) {
        m_suffix->setText(m_params.suffix);
    }

    if (m_params.threshold > 0 && m_params.threshold != 0.5) {
        m_threshold->setText(formatThreshold(m_params.threshold));
        m_thresholdSlider->setValue(realToSliderValue(m_params.threshold));
    }

    if (m_params.savePMap) {
        m_savePMap->setChecked(true);
    }

    if (m_params.savePreProcessing) {
        m_savePreProcessing->setChecked(true);
    }

    if (m_params.skipPreProcessing) {
        m_skipPreProcessing->setChecked(true);
    }

    if (m_params.skipInference) {
        m_skipInference->setChecked(true);
    }

    if (m_params.skipPostProcessing) {
        m_skipPostProcessing->setChecked(true);
    }

    if (!m_params.modelPath.isEmpty()) {
        QString modelName = QFileInfo(m_params.modelPath).baseName();
        int index = m_model->findText(modelName);
        if (index != -1)
            m_model->setCurrentIndex(index);
    }
}

// =========================================================
//                        METHODS
// =========================================================

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_titleBar) {
        if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseMove ||
            event->type() == QEvent::MouseButtonRelease) {
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
    }

    if (obj == m_fileButton) {
        if (event->type() == QEvent::DragEnter) {
            auto *dragEvent = static_cast<QDragEnterEvent *>(event);
            
            if (dragEvent->mimeData()->hasUrls()) {
                m_fileButton->setProperty("dragging", true);
                m_fileButton->update();
                dragEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::DragLeave) {
            m_fileButton->setProperty("dragging", false);
            m_fileButton->update();
            return true;
        } else if (event->type() == QEvent::Drop) {
            auto *dropEvent = static_cast<QDropEvent *>(event);
            const QList<QUrl> urls = dropEvent->mimeData()->urls();

            if (!urls.isEmpty()) {
                QString filePath = urls.first().toLocalFile();

                if (isSupportedFormat(filePath)) {
                    m_fileLabel->setText(QFileInfo(filePath).fileName());

                    if (m_fileChosen)
                        delete m_fileChosen;
                    m_fileChosen = new QString(filePath);

                    dropEvent->acceptProposedAction();
                } else {
                    m_consoleLabel->setText("Unsupported format");
                    dropEvent->ignore();
                }
            }
            m_fileButton->setProperty("dragging", false);
            m_fileButton->update();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::chooseFile() {
    QSettings settings;
    QString lastDirFile = settings.value("lastInputPath", QDir::homePath()).toString();

    QString filters = "Medical Images (*.nii *.nii.gz *.nrrd *.dcm);;All files (*)";

    QString filePath = QFileDialog::getOpenFileName(this, "Choose file", lastDirFile, filters);

    if (!filePath.isEmpty()) {
        m_fileLabel->setText(QFileInfo(filePath).fileName());
        m_fileChosen = new QString(filePath);
        settings.setValue("lastInputPath", QFileInfo(filePath).absolutePath());
    }
}

bool MainWindow::isSupportedFormat(const QString &filePath) {
    QFileInfo info(filePath);

    if (info.isDir())
        return true;

    static const QStringList supportedExtensions = {"nii", "nii.gz", "nrrd", "dcm"};

    QString suffix = info.suffix().toLower();
    QString completeSuffix = info.completeSuffix().toLower();

    return supportedExtensions.contains(suffix) || supportedExtensions.contains(completeSuffix);
}

void MainWindow::chooseDestination() {
    QSettings settings;
    QString lastDest = settings.value("lastDestPath", QDir::homePath()).toString();

    QString folderPath = QFileDialog::getExistingDirectory(
        this,"Choose output folder", lastDest,QFileDialog::ShowDirsOnly);

    if (!folderPath.isEmpty()) {
        m_destination->setText(folderPath);
        settings.setValue("lastDestPath", folderPath);
    }
}

void MainWindow::openGuide() {
    if (guide.isNull())
        guide = new GuideWindow();

    guide->show();
    guide->raise();
    guide->activateWindow();
}

void MainWindow::openAbout() {
    if (about.isNull())
        about = new AboutWindow();

    about->show();
    about->raise();
    about->activateWindow();
}

void MainWindow::openModelManager() {
    if (modelManager.isNull()) {
        modelManager = new ModelManager(this);
        connect(modelManager, &ModelManager::modelsChanged, this, &MainWindow::refreshModelsList);
    }

    modelManager->show();
    modelManager->raise();
    modelManager->activateWindow();
    modelManager->move(modelManager->width() + 32,
                       this->geometry().bottom() - modelManager->height() - 15);
}


void MainWindow::refreshModelsList() {
    QString currentModel = m_model->currentText();
    m_model->clear();

    QDir modelsDir("C:/ProgramData/StrokeSeg/Model");
    QStringList entries = modelsDir.entryList(QDir::Files | QDir::NoDotAndDotDot);

    for (const QString &entry : entries) {
        m_model->addItem(QFileInfo(entry).baseName());
    }

    int index = m_model->findText(currentModel);
    if (index != -1)
        m_model->setCurrentIndex(index);
}


double MainWindow::sliderValueToReal(int v) {
    if (v <= 0) {
        return 1e-5;
    }
    if (v <= 8) {
        return 1e-4;
    }
    if (v < 16)  {
        return 1e-3;
    }
    if (v == 16) {
        return 0.01;
    }
    if (v >= 100) {
        return 1.0 - 1e-5;
    }
    if (v >= 92) {
        return 1.0 - 1e-4;
    }
    if (v > 84) {
        return 1.0 - 1e-3;
    }
    if (v == 84) {
        return 0.99;
    }

    double t = (v - 16) / 80.0;
    return 0.001 + t * (0.999 - 0.001);
}

int MainWindow::realToSliderValue(double v) {
    if (v <= 1e-5) {
        return 0;
    }
    if (v <= 1e-4) {
        return 8;
    }
    if (v <= 1e-3) {
        return 16;
    }
    if (v >= 1.0 - 1e-5) {
        return 100;
    }
    if (v >= 1.0 - 1e-4) {
        return 92;
    }
    if (v >= 1.0 - 1e-3) {
        return 84;
    }

    double t = (v - 0.001) / (0.999 - 0.001);
    return 16 + int(std::round(t * 68));
}

QString MainWindow::formatThreshold(double v) {
    if (v <= 1e-5)          
        return "10\u207B\u2075";    
    if (v <= 1e-4)          
        return "10\u207B\u2074";
    if (v <= 1e-3)          
        return "0.001";             
    if (v >= 1.0 - 1e-5)    
        return "1-10\u207B\u2075";
    if (v >= 1.0 - 1e-4)    
        return "1-10\u207B\u2074";  
    if (v >= 1.0 - 1e-3)    
        return "0.999";

    return QString::number(v, 'f', 2);
}

void MainWindow::setInputsEnabled(bool enabled) {
    m_suffix->setEnabled(enabled);
    m_destination->setEnabled(enabled);
    m_destinationButton->setEnabled(enabled);
    m_model->setEnabled(enabled);
    m_toggleView->setEnabled(enabled);
    m_toggleOpenFolder->setEnabled(enabled);
    m_toggleOutput->setEnabled(enabled);
    m_skipPreProcessing->setEnabled(enabled);
    m_skipInference->setEnabled(enabled);
    m_skipPostProcessing->setEnabled(enabled);
    m_savePMap->setEnabled(enabled);
    m_savePreProcessing->setEnabled(enabled);
    m_mode->setEnabled(enabled);
    m_thresholdSlider->setEnabled(enabled);
    m_threshold->setEnabled(enabled);
    m_runButton->setEnabled(enabled);
    m_fileButton->setEnabled(enabled);
}

void MainWindow::Process() {

    if (*m_fileChosen == "Choose File")
        return;

    if (m_destination->text().isEmpty()) {
        m_consoleLabel->setText("Please select an output folder.");
        return;
    }

    m_stackedArea->setCurrentIndex(1);

    m_runButton->setEnabled(false);

    m_params.t1Path = *m_fileChosen;
    m_params.outputDir = m_destination->text() + "/" + QFileInfo(*m_fileChosen).baseName();
    m_params.modelPath = Paths::modelDir().filePath(m_model->currentText() + ".onnx");
    m_params.suffix = m_suffix->text();
    m_params.savePMap = m_savePMap->isChecked();
    m_params.savePreProcessing = m_savePreProcessing->isChecked();
    m_params.skipPreProcessing = m_skipPreProcessing->isChecked();
    m_params.skipInference = m_skipInference->isChecked();
    m_params.skipPostProcessing = m_skipPostProcessing->isChecked();
    m_params.mni = m_toggleOutput->isChecked();
    m_params.betOnly = m_mode->currentText() == "Brain Extraction Only";

    if (QDir().mkpath(m_params.outputDir)) {
        qDebug() << "Output directory created:" << m_params.outputDir;
    } else {
        qDebug() << "Failed to create output directory:" << m_params.outputDir;
    }

    m_params.threshold = m_threshold->text().replace(" ", "").toFloat();

    setInputsEnabled(false);
    m_stackedArea->setCurrentIndex(1);
    m_consoleLabel->setText("Starting pipeline...");

    QThread *thread = new QThread;
    PipelineWorker *worker = new PipelineWorker(m_params);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &PipelineWorker::process);

    connect(worker, &PipelineWorker::statusChanged, this,
            [this](QString msg) { m_consoleLabel->setText(msg); });

    connect(worker, &PipelineWorker::finished, this, &MainWindow::onPipelineFinished);

    connect(worker, &QObject::destroyed, thread, &QThread::quit);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void MainWindow::onPipelineFinished(bool success, QString message, QString finalPath) {
    setInputsEnabled(true);
    m_stackedArea->setCurrentIndex(0);
    m_consoleLabel->setText(message);

    sender()->deleteLater();

    if (success) {
        if (m_toggleOpenFolder->isChecked()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_destination->text()));
        }

        if (m_toggleView->isChecked() && !finalPath.isEmpty()) {
            // We pass the original MRI (m_params.t1Path) AND the generated mask (finalPath)
            QString baseImagePath;

            if (m_params.betOnly) {
                finalPath = "";
                baseImagePath = m_params.outputDir + "/" +
                                QFileInfo(m_params.t1Path).baseName() + "_BET.nii.gz";
            }

            if (m_params.mni) {
                baseImagePath = m_params.outputDir + "/" +
                                "MNI_" + QFileInfo(m_params.t1Path).baseName() + "_BET.nii.gz";
            } else {
                baseImagePath = m_params.t1Path;
            }

            NiftiViewerWindow *viewer = new NiftiViewerWindow(baseImagePath, finalPath, this);
            qDebug() << "Opening viewer with MRI:" << baseImagePath << "and mask:" << finalPath;

            // Qt will automatically delete the window from memory when the user closes it
            viewer->setAttribute(Qt::WA_DeleteOnClose);

            // Force it to open as an independent window, not embedded inside the main UI
            viewer->setWindowFlag(Qt::Window);
            viewer->show();
        }
    } else {
        QMessageBox::critical(this, "Pipeline Error", message);
    }
}

void MainWindow::saveSettings() {
    QSettings settings;

    // Form fields
    settings.setValue("suffix", m_suffix->text());
    settings.setValue("destination", m_destination->text());
    settings.setValue("modelIndex", m_model->currentIndex());
    settings.setValue("executionMode", m_mode->currentIndex());

    // Checkboxes
    settings.setValue("toggleView", m_toggleView->isChecked());
    settings.setValue("toggleOpenFolder", m_toggleOpenFolder->isChecked());
    settings.setValue("toggleOutput", m_toggleOutput->isChecked());
    settings.setValue("skipPreProcessing", m_skipPreProcessing->isChecked());
    settings.setValue("skipInference", m_skipInference->isChecked());
    settings.setValue("skipPostProcessing", m_skipPostProcessing->isChecked());
    settings.setValue("savePMap", m_savePMap->isChecked());
    settings.setValue("savePreProcessing", m_savePreProcessing->isChecked());

    // Threshold
    settings.setValue("thresholdValue", m_threshold->text());
    settings.setValue("thresholdSlider", m_thresholdSlider->value());
}

void MainWindow::loadSettings() {
    QSettings settings;

    m_suffix->setText(settings.value("suffix", "").toString());
    m_destination->setText(settings.value("destination", "").toString());

    // On restaure l'index du mod�le seulement s'il est valide
    int modelIdx = settings.value("modelIndex", 0).toInt();
    if (modelIdx < m_model->count())
        m_model->setCurrentIndex(modelIdx);

    m_mode->setCurrentIndex(settings.value("executionMode", 0).toInt());

    m_toggleView->setChecked(settings.value("toggleView", false).toBool());
    m_toggleOpenFolder->setChecked(settings.value("toggleOpenFolder", false).toBool());
    m_toggleOutput->setChecked(settings.value("toggleOutput", false).toBool());
    m_skipPreProcessing->setChecked(settings.value("skipPreProcessing", false).toBool());
    m_skipInference->setChecked(settings.value("skipInference", false).toBool());
    m_skipPostProcessing->setChecked(settings.value("skipPostProcessing", false).toBool());
    m_savePMap->setChecked(settings.value("savePMap", false).toBool());
    m_savePreProcessing->setChecked(settings.value("savePreProcessing", false).toBool());

    m_threshold->setText(settings.value("thresholdValue", "0.50").toString());
    m_thresholdSlider->setValue(settings.value("thresholdSlider", 50).toInt());
}

void MainWindow::toggleConsole() {
#ifdef Q_OS_WIN
    HWND hwnd = GetConsoleWindow();

    if (hwnd == NULL) {
        if (AllocConsole()) {
            freopen("CONOUT$", "w", stdout);
            freopen("CONOUT$", "w", stderr);
            std::ios::sync_with_stdio();
            hwnd = GetConsoleWindow();
        }
    }

    HMENU hMenu = GetSystemMenu(hwnd, FALSE);
    if (hMenu) {
        DeleteMenu(hMenu, SC_CLOSE, MF_BYCOMMAND);
        DrawMenuBar(hwnd);
    }

    static bool isVisible = false;

    if (hwnd) {
        if (!isVisible) {
            ShowWindow(hwnd, SW_SHOW);
            SetForegroundWindow(hwnd);
            m_terminalButton->setText("Hide Console");
            isVisible = true;

            QString logPath = LogManager::getLogFilePath();
            QFile logFile(logPath);
            if (logFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&logFile);
                system("cls");
                printf("%s\n", in.readAll().toLocal8Bit().constData());
                fflush(stdout);
                logFile.close();
            }

            qDebug() << "--- Console Session Active ---";
        } else {
            ShowWindow(hwnd, SW_HIDE);
            m_terminalButton->setText("Show Console");
            isVisible = false;
        }
    }
#endif
}

void MainWindow::closeEvent(QCloseEvent *event) {

    if (!modelManager.isNull()) {
        modelManager->close();
    }

    if (!guide.isNull()) {
        guide->close();
    }

    if (!about.isNull()) {
        about->close();
    }

    if (!m_params.gui) {
        saveSettings();
    }
    event->accept();
}

MainWindow::~MainWindow() {
    if (!m_params.gui) {
        saveSettings();
    }
}