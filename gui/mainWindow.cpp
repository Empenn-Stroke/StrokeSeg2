#include "mainWindow.h"

#include <QIcon>
#include <algorithm>
#include <QSettings>
#include <QStandardPaths>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>
#include <QtConcurrent>
#include <QMovie>

#include <../core/inference/inferenceengine.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    

    setWindowTitle("StrokeSeg2");
    setWindowIcon(QIcon("../../../gui/ressources/StrokeSeg2.ico"));
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
    windowLayout->setContentsMargins(15, 15, 15, 15);
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
    QDir modelsDir = QDir("C:/ProgramData/StrokeSeg/Models");
    QStringList entries = modelsDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    for (const QString &entry : entries) {
        QFileInfo info(modelsDir.filePath(entry));
        QString displayName = info.baseName();
        m_model->addItem(displayName);
    }

    // Toggle
    m_toggleView = new QCheckBox("", formParameters);
    m_toggleOutput = new QCheckBox("", formParameters);
    m_skipBrainExtract = new QCheckBox("", formParameters);
    m_savePMap = new QCheckBox("", formParameters);
    m_savePreprocessing = new QCheckBox("", formParameters);

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

    QWidget *thresholdContainer = new QWidget(formParameters);
    thresholdContainer->setObjectName("thresholdContainer");
    thresholdContainer->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *thresholdLayout = new QHBoxLayout(thresholdContainer);
    thresholdLayout->setContentsMargins(0, 0, 0, 0);
    thresholdLayout->addWidget(m_threshold);
    thresholdLayout->addWidget(m_thresholdSlider);
    thresholdLayout->setSpacing(8);


    // Assembly
    m_formLayout->addRow("Suffix :", m_suffix);

    QLabel *destionationLabel = new QLabel("Destination :", formParameters);
    m_formLayout->addRow(destionationLabel, destinationContainer);

    m_formLayout->addRow("Model :", m_model);
    m_formLayout->addRow("Open viewer :", m_toggleView);
    m_formLayout->addRow("Output MNI space :", m_toggleOutput);
    m_formLayout->addRow("Skip brain extraction:", m_skipBrainExtract);
    m_formLayout->addRow("Save probability map :", m_savePMap);
    m_formLayout->addRow("Save pre-processing :", m_savePreprocessing);
    m_formLayout->addRow("Execution mode :", m_mode);

    QLabel *thresholdLabel = new QLabel("Threshold :", formParameters);
    m_formLayout->addRow(thresholdLabel, thresholdContainer);


    // ---------------- BOTTOM BUTTONS ----------------

    QWidget *bottomBtns = new QWidget(leftColumn);
    bottomBtns->setObjectName("bottomBtns");
    bottomBtns->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBtns);

    m_modelManager = new QPushButton("Model manager");
    m_resetSettings = new QPushButton("Reset settings");

    bottomLayout->addWidget(m_modelManager);
    bottomLayout->addWidget(m_resetSettings);

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
    QPixmap pix("../../../gui/ressources/files.png");
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
    m_consoleLabel->setText("Console output...");
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
    QLabel *spinnerLabel = new QLabel(loadingPage);
    QMovie *movie = new QMovie("../../../gui/ressources/infinite-spinner-optimized.gif");
    movie->start();
    spinnerLabel->setMovie(movie);
    loadingLayout->addStretch();
    loadingLayout->addWidget(spinnerLabel, 0, Qt::AlignHCenter);
    loadingLayout->addStretch();

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
    connect(m_resetSettings, &QPushButton::clicked, this, [this]() { 
        m_suffix->setText("");
        m_destination->setText("");
        m_toggleView->setChecked(false);
        m_toggleOutput->setChecked(false);
        m_savePMap->setChecked(false);
        m_savePreprocessing->setChecked(false);
        m_threshold->setText("0.50");
        m_thresholdSlider->setValue(50);
        m_model->setCurrentIndex(1);
        m_mode->setCurrentIndex(0);
        m_skipBrainExtract->setChecked(false);

        m_consoleLabel->setText("Settings reset");
    });
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(m_runButton, &QPushButton::clicked, this, &MainWindow::Process);

    connect(m_thresholdSlider, &QSlider::valueChanged, this, [this](int v) {
        double realVal = sliderValueToReal(v);
        m_threshold->setText(formatThreshold(realVal));
    });

    connect(m_threshold, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool ok;
        double val = 0.0;

        if (text == "1-10\u207B\u2075") val = 1.0 - 1e-5;
        else if (text == "10\u207B\u2075") val = 1e-5;
        else if (text == "1-10\u207B\u2074") val = 1.0 - 1e-4;
        else if (text == "10\u207B\u2074") val = 1e-4;
        else {
            val = text.toDouble(&ok);
            if (!ok)
                return;
        }

        int sliderVal = realToSliderValue(val);

        if (sliderVal == m_thresholdSlider->value()) {
            m_thresholdSlider->blockSignals(true);
            m_thresholdSlider->setValue(sliderVal);
            m_thresholdSlider->blockSignals(false);
        }
    });

    connect(actionGuide, &QAction::triggered, this, &MainWindow::openGuide);

    connect(actionResetWW, &QAction::toggled, this, [this](bool checked) {
        showWarning = checked;
        QSettings settings;
        settings.setValue("showWarning", showWarning);
    });
    
    connect(m_mode, &QComboBox::currentIndexChanged, this, [this, thresholdLabel](int index) {
        m_formLayout->setRowVisible(thresholdLabel, index != 1);
    });

    loadSettings();
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

                if (filePath.endsWith(".nii") || filePath.endsWith(".nii.gz")) {
                    //m_fileButton->setText(QFileInfo(filePath).fileName());
                    m_fileLabel->setText(QFileInfo(filePath).fileName());
                    m_fileChosen = new QString(filePath);
                    dropEvent->acceptProposedAction();
                } else {
                    m_consoleLabel->setText("File format not supported");
                    dropEvent->ignore();
                }
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::chooseFile() {

    QSettings settings;
    QString lastDirFile = settings.value("lastInputPath", QDir::homePath()).toString();

    QString filePath = QFileDialog::getOpenFileName(
        this, 
        "Choose file",
        lastDirFile,
        "IRM images (*.nii *.nii.gz);;All files (*)"
    );

    if (!filePath.isEmpty()) {
        m_fileLabel->setText(QFileInfo(filePath).fileName());
        m_fileChosen = new QString(filePath);
        settings.setValue("lastInputPath", QFileInfo(filePath).absolutePath());
    }
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

void MainWindow::importModel() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Choose file", "", "ONNX Model (*.onnx);;All files (*)");

    if (filename.isEmpty())
        return;

    // ---- Find the path ----
    QString programDataPath = qgetenv("PROGRAMDATA");
    if (programDataPath.isEmpty()) {
        programDataPath = "C:/ProgramData"; // Fallback manuel si la variable est vide
    }
    QDir dir(programDataPath + "/StrokeSeg/Models");

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << ("Critical Error: Failed to create directory in ProgramData.");
            return;
        }
    }

    QString destFile = dir.filePath(QFileInfo(filename).fileName());

    // ---- Clear if already exists ----
    if (QFile::exists(destFile)) {
        if (!QFile::remove(destFile)) {
            qDebug() << ("Error: Unable to overwrite existing file (Access denied).");
            return;
        }
    }

    bool success = false;
    QString methodUsed = "";

    // ---- Try to hardlink ----
#ifdef Q_OS_WIN
    std::wstring src = filename.toStdWString();
    std::wstring dst = destFile.toStdWString();

    if (CreateHardLinkW(dst.c_str(), src.c_str(), NULL)) {
        success = true;
        methodUsed = "Hardlink";
    } else
        qDebug() << "Hardlink has failed (Probably not the same disk). Trying to copy...";
#endif

    // ---- Standard copy if the hardlink failed ----
    if (!success) {
        if (QFile::copy(filename, destFile)) {
            success = true;
            methodUsed = "Standard copy";
        }
    }

    // ---- Result ----
    if (success) {
        qDebug() << "Success ! " << methodUsed << " created at : " << destFile;
    } else {
        qDebug() << "Total failure. Do verify admin access.";
    }

    QFileInfo info(destFile);
    QString displayName = info.baseName();
    m_model->addItem(displayName);
}

void MainWindow::openGuide() {
    if (guide.isNull())
        guide = new GuideWindow();

    guide->show();
    guide->raise();
    guide->activateWindow();
}

void MainWindow::openModelManager() {
    if (modelManager.isNull())
        modelManager = new ModelManager();

    modelManager->show();
    modelManager->raise();
    modelManager->activateWindow();
}

double MainWindow::sliderValueToReal(int v) {
    if (v <= 0)     return 1e-5;            if (v <= 8)     return 1e-4;        
    if (v < 16)     return 1e-3;            if (v == 16)    return 0.01;
    if (v >= 100)   return 1.0 - 1e-5;      if (v >= 92)    return 1.0 - 1e-4; 
    if (v > 84)     return 1.0 - 1e-3;      if (v == 84)    return 0.99;

    double t = (v - 10) / 80.0;
    return 0.01 + t * (0.99 - 0.01);
}

int MainWindow::realToSliderValue(double v) {
    if (v <= 1e-5)          return 0;       if (v <= 1e-4)          return 8;
    if (v <= 1e-3)          return 16;      if (v >= 1.0 - 1e-5)    return 100;
    if (v >= 1.0 - 1e-4)    return 92;      if (v >= 1.0 - 1e-3)    return 84;

    double t = (v - 0.01) / (0.99 - 0.01);
    return 32 + int(std::round(t * 68));
}

QString MainWindow::formatThreshold(double v) {
    if (v <= 1e-5)          return "10\u207B\u2075";    if (v <= 1e-4)          return "10\u207B\u2074";
    if (v <= 1e-3)          return "0.001";             if (v >= 1.0 - 1e-5)    return "1-10\u207B\u2075";
    if (v >= 1.0 - 1e-4)    return "1-10\u207B\u2074";  if (v >= 1.0 - 1e-3)    return "0.999";

    return QString::number(v, 'f', 2);
}

void MainWindow::Process() {

    if (m_destination->text() == "" || *m_fileChosen == "Choose File")
        return;

    m_stackedArea->setCurrentIndex(1);

    m_runButton->setEnabled(false);
    m_consoleLabel->setText("Running inference...");

    QString modelPath = "C:/ProgramData/StrokeSeg/Models/" + m_model->currentText() + ".onnx";
    QString imagePath = *m_fileChosen;
    QString destinationPath =
        m_destination->text() + "/" + m_suffix->text() + QFileInfo(*m_fileChosen).fileName();

    // Start the computation in another thread
    QFuture<std::vector<float>> worker = QtConcurrent::run([modelPath, imagePath,destinationPath]() {
        InferenceEngine engine;
        return engine.RunInference(modelPath, imagePath, destinationPath);
    });

    // Watch for the end of the computation
    auto watcher = new QFutureWatcher<std::vector<float>>();
    connect(watcher, &QFutureWatcher<std::vector<float>>::finished, this, [this, watcher,destinationPath]() {
        auto output = watcher->result();
        if (output.empty()) {
            m_consoleLabel->setText("Failure : no data out");
        } else {
            m_consoleLabel->setText(QString("Success! Output size = %1").arg(output.size()));

            // ITK
            if (m_toggleView->isChecked()) {
                QStringList arguments;
                arguments << "-g" << destinationPath;

                bool started = QProcess::startDetached("itksnap", arguments);

                if (!started) {
                    QString commonPath = "C:/Program Files/ITK-SNAP 4.4/bin/ITK-SNAP.exe";
                    if (!QProcess::startDetached(commonPath, arguments)) {
                        m_consoleLabel->setText("Success, but ITK-SNAP not found.");
                    }
                }
            }

        }

        m_stackedArea->setCurrentIndex(0);
        m_runButton->setEnabled(true);
        watcher->deleteLater();
    });
    watcher->setFuture(worker);
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
    settings.setValue("toggleOutput", m_toggleOutput->isChecked());
    settings.setValue("skipBrainExtract", m_skipBrainExtract->isChecked());
    settings.setValue("savePMap", m_savePMap->isChecked());
    settings.setValue("savePreprocessing", m_savePreprocessing->isChecked());

    // Threshold
    settings.setValue("thresholdValue", m_threshold->text());
    settings.setValue("thresholdSlider", m_thresholdSlider->value());
}

void MainWindow::loadSettings() {
    QSettings settings;

    m_suffix->setText(settings.value("suffix", "").toString());
    m_destination->setText(settings.value("destination", "").toString());

    // On restaure l'index du modèle seulement s'il est valide
    int modelIdx = settings.value("modelIndex", 0).toInt();
    if (modelIdx < m_model->count())
        m_model->setCurrentIndex(modelIdx);

    m_mode->setCurrentIndex(settings.value("executionMode", 0).toInt());

    m_toggleView->setChecked(settings.value("toggleView", false).toBool());
    m_toggleOutput->setChecked(settings.value("toggleOutput", false).toBool());
    m_skipBrainExtract->setChecked(settings.value("skipBrainExtract", false).toBool());
    m_savePMap->setChecked(settings.value("savePMap", false).toBool());
    m_savePreprocessing->setChecked(settings.value("savePreprocessing", false).toBool());

    m_threshold->setText(settings.value("thresholdValue", "0.50").toString());
    m_thresholdSlider->setValue(settings.value("thresholdSlider", 50).toInt());
}

MainWindow::~MainWindow() {
    saveSettings();
}