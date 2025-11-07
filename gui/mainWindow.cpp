#include "mainWindow.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {

    setWindowTitle("StrokeSeg2");
    QIcon icon("../../../gui/ressources/StrokeSeg2.ico");
    setWindowIcon(icon);
    resize(1280, 720);

    m_mainWidget = new QWidget(this);
    setCentralWidget(m_mainWidget);
    QHBoxLayout *mainLayout = new QHBoxLayout(m_mainWidget);


    // Colonne à gauche

    QWidget *leftContainer = new QWidget(m_mainWidget);
    leftContainer->setObjectName("leftContainer");

    QVBoxLayout *leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setObjectName("leftLayout");

    QWidget *leftPanel = new QWidget(leftContainer);
    leftPanel->setObjectName("colonneGauche");

    QFormLayout *formLayout = new QFormLayout(leftPanel);


    m_suffix = new QLineEdit(leftPanel);
    m_suffix->setPlaceholderText("Enter the suffix name");

    m_model = new QComboBox(leftPanel);
    m_model->addItem("Option 1");
    m_model->addItem("Option 2");
    m_model->addItem("Option 3");

    m_toggleView = new QCheckBox("", leftPanel);
    m_toggleOutput = new QCheckBox("", leftPanel);

    m_mode = new QComboBox(leftPanel);
    m_mode->addItem("Option 1");
    m_mode->addItem("Option 2");
    m_mode->addItem("Option 3");

    formLayout->addRow("Suffix :", m_suffix);
    formLayout->addRow("Model :", m_model);
    formLayout->addRow("Open viewer :", m_toggleView);
    formLayout->addRow("Output MNI space :", m_toggleOutput);
    formLayout->addRow("Mode :", m_mode);

    formLayout->setAlignment(m_suffix, Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setAlignment(m_model, Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setAlignment(m_toggleView, Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setAlignment(m_toggleOutput, Qt::AlignRight | Qt::AlignVCenter);
    formLayout->setAlignment(m_mode, Qt::AlignRight | Qt::AlignVCenter);


    leftLayout->addStretch(1);
    leftLayout->addWidget(leftPanel,0);
    leftLayout->addStretch(10);


    // Partie principale à droite

    QWidget *mainArea = new QWidget(m_mainWidget);
    mainArea->setObjectName("mainArea");

    QVBoxLayout *mainAreaLayout = new QVBoxLayout(mainArea);

    m_fileButton = new QToolButton(mainArea);
    m_fileButton->setObjectName("chooseFileButton");
    m_fileButton->setText("Choose file");
    QIcon files_icon("../../../gui/ressources/files.png");
    m_fileButton->setIcon(files_icon);
    m_fileButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_runButton = new QPushButton("RUN", mainArea);

    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(m_fileButton, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(m_runButton, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();


    // Construction finale de la window

    mainLayout->addWidget(leftContainer);
    mainLayout->addWidget(mainArea);

    mainLayout->setStretch(0, 3);
    mainLayout->setStretch(1, 7);

    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    leftLayout->setSpacing(10);
    leftLayout->setContentsMargins(10, 10, 10, 10);

    formLayout->setSpacing(8);
    formLayout->setContentsMargins(0, 0, 0, 0);

    mainAreaLayout->setSpacing(20);
    mainAreaLayout->setContentsMargins(0, 0, 0, 0);

    connect(m_fileButton, &QToolButton::clicked, this, &MainWindow::chooseFile);
}

MainWindow::~MainWindow() {}

void MainWindow::chooseFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Choose file");
    if (!filename.isEmpty()) {
        m_fileButton->setText(filename);
    }
}
