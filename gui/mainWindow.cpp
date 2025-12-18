#include "mainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

    setWindowTitle("StrokeSeg2");
    setWindowIcon(QIcon("../../../gui/ressources/StrokeSeg2.ico"));
    resize(1280, 720);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    QSettings settings;
    showWarning = settings.value("showWarning", true).toBool();

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    m_mainWidget = new QWidget(this);
    m_mainWidget->setObjectName("m_mainWidget");
    m_mainWidget->setAttribute(Qt::WA_StyledBackground, true);
    setCentralWidget(m_mainWidget);

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

    QMenu *optionsMenu = menuBar->addMenu("Model");
    optionsMenu->addAction("Import model");
    optionsMenu->addAction("Export model");

    QMenu *helpMenu = menuBar->addMenu("Help");
    QAction *actionGuide = new QAction("Guide", this);
    helpMenu->addAction(actionGuide);
    QAction *actionAbout = new QAction("About", this);
    helpMenu->addAction(actionAbout);
    QAction *actionResetWW = new QAction("Show warning", this);
    actionResetWW->setCheckable(true);
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
    leftLayout->setSpacing(10);

    // ---------------- FORM ----------------

    QWidget *formParameters = new QWidget(leftColumn);
    formParameters->setObjectName("formParameters");
    formParameters->setAttribute(Qt::WA_StyledBackground, true);

    QFormLayout *formLayout = new QFormLayout(formParameters);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(8);

    // Suffix
    m_suffix = new QLineEdit(formParameters);
    m_suffix->setObjectName("suffix");
    m_suffix->setPlaceholderText("Enter the suffix name");

    // Model
    m_model = new QComboBox(formParameters);
    m_model->addItems({"Monomodal (T1)", "Bimodal (T1 + flair)"});

    // Toggle Open Viewer
    m_toggleView = new QCheckBox("", formParameters);
    m_toggleOutput = new QCheckBox("", formParameters);

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
    formLayout->addRow("Suffix :", m_suffix);
    formLayout->addRow("Model :", m_model);
    formLayout->addRow("Open viewer :", m_toggleView);
    formLayout->addRow("Output MNI space :", m_toggleOutput);
    formLayout->addRow("Execution mode :", m_mode);
    formLayout->addRow("Threshold :", thresholdContainer);


    // ---------------- BOTTOM BUTTONS ----------------

    QWidget *bottomBtns = new QWidget(leftColumn);
    bottomBtns->setObjectName("bottomBtns");
    bottomBtns->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBtns);
    m_savePMap = new QPushButton("Save probability map");
    m_savePreprocessing = new QPushButton("Save pre-processing");

    bottomLayout->addWidget(m_savePMap);
    bottomLayout->addWidget(m_savePreprocessing);

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
    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);

    connect(m_thresholdSlider, &QSlider::valueChanged, this,
            [this](int v) { m_threshold->setText(QString::number(v * 0.01f, 'f', 2)); });

    connect(m_threshold, &QLineEdit::textChanged, this, [this](const QString &text) {
        bool ok;
        float value = text.toFloat(&ok);
        if (!ok)
            return;

        value = std::clamp(value, 0.0f, 1.0f);
        m_thresholdSlider->blockSignals(true);
        m_thresholdSlider->setValue(int(value * 100));
        m_thresholdSlider->blockSignals(false);
    });

    connect(actionGuide, &QAction::triggered, this, &MainWindow::openGuide);
    connect(actionResetWW, &QAction::triggered, this, [this, actionResetWW]() {
        showWarning = !showWarning;
        QSettings settings;
        settings.setValue("showWarning", showWarning);
        actionResetWW->setChecked(showWarning);
    });
    
    // Warning window
    if (showWarning) {
        warning = new WarningWindow();
        warning->exec();
    }

}

// =========================================================
//                        METHODS
// =========================================================

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_titleBar) {
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
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::chooseFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Choose file");
    if (!filename.isEmpty())
        m_fileButton->setText(filename);
}

void MainWindow::openGuide() {
    if (!guide)
        guide = new GuideWindow();

    guide->show();
    guide->raise();
    guide->activateWindow();
}

MainWindow::~MainWindow() {}
