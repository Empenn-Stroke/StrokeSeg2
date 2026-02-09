#include "mainWindow.h"

#include <QIcon>
#include <algorithm>
#include <QSettings>
#include <QStandardPaths>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QDebug>

#include <utils/path.h>

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

    // Model
    m_model = new QComboBox(formParameters);
    m_model->addItems({"Monomodal (T1)", "Bimodal (T1 + FLAIR)"});

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


    // Assembly
    m_formLayout->addRow("Suffix :", m_suffix);
    m_formLayout->addRow("Model :", m_model);
    m_formLayout->addRow("Open viewer :", m_toggleView);
    m_formLayout->addRow("Output MNI space :", m_toggleOutput);
    m_formLayout->addRow("Skip brain extraction:", m_savePMap);
    m_formLayout->addRow("Save probability map :", m_skipBrainExtract);
    m_formLayout->addRow("Save pre-processing :", m_savePreprocessing);
    m_formLayout->addRow("Execution mode :", m_mode);

    QLabel *thresholdLabel = new QLabel("Threshold :", formParameters);
    m_formLayout->addRow(thresholdLabel, thresholdContainer);


    // ---------------- BOTTOM BUTTONS ----------------

    QWidget *bottomBtns = new QWidget(leftColumn);
    bottomBtns->setObjectName("bottomBtns");
    bottomBtns->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBtns);

    m_importModel = new QPushButton("Import model");

    bottomLayout->addWidget(m_importModel);

    leftLayout->addStretch(1);
    leftLayout->addWidget(formParameters);
    leftLayout->addStretch(25);
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

    m_fileButton = new QToolButton(mainArea);
    m_fileButton->setObjectName("chooseFileButton");
    m_fileButton->setText("Choose file");
    m_fileButton->setIcon(QIcon("../../../gui/ressources/files.png"));
    m_fileButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    m_fileButton->setAcceptDrops(true);
    m_fileButton->installEventFilter(this);
    m_fileButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_runButton = new QPushButton("RUN", mainArea);
    m_runButton->setObjectName("runBtn");

    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(m_fileButton, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(m_runButton, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();

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
    connect(m_importModel, &QToolButton::clicked, this, &MainWindow::importModel);
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);

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
            bool ok;
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
                    m_fileButton->setText(QFileInfo(filePath).fileName());
                    dropEvent->acceptProposedAction();
                } else {
                    qDebug() << "File format not supported";
                    dropEvent->ignore();
                }
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::chooseFile() {
    QString filepath = QFileDialog::getOpenFileName(this, 
                                                    "Choose file",
                                                    "",
                                                    "IRM images (*.nii *.nii.gz);;All files (*)");
    if (!filepath.isEmpty())
        m_fileButton->setText(QFileInfo(filepath).fileName());
}

void MainWindow::importModel() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Choose file", "", "ONNX Model (*.onnx);;All files (*)");

    if (filename.isEmpty())
        return;

    // 1. Définir le chemin de destination dans ProgramData
    
    // appdata
    //QString programDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

    QString programDataPath = qgetenv("PROGRAMDATA");
    if (programDataPath.isEmpty()) {
        programDataPath = "C:/ProgramData"; // Fallback manuel si la variable est vide
    }
    QDir dir(programDataPath + "/StrokeSeg/Models");

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "Critical error : Impossible to create the directory in ProgramData.";
            return;
        }
    }

    QString destFile = dir.filePath(QFileInfo(filename).fileName());

    // 2. Nettoyer si un fichier existe déjà
    if (QFile::exists(destFile)) {
        if (!QFile::remove(destFile)) {
            qDebug() << "Impossible to replace the existant file (no access).";
            return;
        }
    }

    bool success = false;
    QString methodUsed = "";

    // 3. Tentative de Hardlink (Windows seulement)
#ifdef Q_OS_WIN
    std::wstring src = filename.toStdWString();
    std::wstring dst = destFile.toStdWString();

    if (CreateHardLinkW(dst.c_str(), src.c_str(), NULL)) {
        success = true;
        methodUsed = "Hardlink";
    } else
        qDebug() << "Hardlink has failed (Probably not the same disk). Trying to copy...";
#endif

    // 4. Fallback : Copie classique si le hardlink a échoué ou si on est pas sur Windows
    if (!success) {
        if (QFile::copy(filename, destFile)) {
            success = true;
            methodUsed = "Standard copy";
        }
    }

    // 5. Résultat
    if (success) {
        qDebug() << "Success ! " << methodUsed << " created at : " << destFile;
    } else {
        qDebug() << "Total failure. Do verify admin access.";
    }
}

void MainWindow::openGuide() {
    if (!guide)
        guide = new GuideWindow();

    guide->show();
    guide->raise();
    guide->activateWindow();
}

MainWindow::~MainWindow() {}

double MainWindow::sliderValueToReal(int v) {
    if (v <= 0) return 1e-5;
    if (v <= 8) return 1e-4;
    if (v < 16) return 1e-3;
    if (v == 16) return 0.01;

    if (v >= 100) return 1.0 - 1e-5;
    if (v >= 92) return 1.0 - 1e-4;
    if (v > 84) return 1.0 - 1e-3;
    if (v == 84)  return 0.99;

    double t = (v - 10) / 80.0;
    return 0.01 + t * (0.99 - 0.01);
}

int MainWindow::realToSliderValue(double v) {
    if (v <= 1e-5) return 0;
    if (v <= 1e-4) return 8;
    if (v <= 1e-3) return 16;

    if (v >= 1.0 - 1e-5) return 100;
    if (v >= 1.0 - 1e-4) return 92;
    if (v >= 1.0 - 1e-3) return 84;

    double t = (v - 0.01) / (0.99 - 0.01);
    return 32 + int(std::round(t * 68));
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
