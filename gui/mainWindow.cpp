#include "mainWindow.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QFormLayout>
#include <QMenuBar>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("StrokeSeg2");
    QIcon icon("../../../gui/ressources/StrokeSeg2.ico");
    setWindowIcon(icon);
    resize(1000, 600);

    m_mainWidget = new QWidget(this);
    setCentralWidget(m_mainWidget);

    QHBoxLayout *mainLayout = new QHBoxLayout(m_mainWidget);

    //QMenuBar *menuBar = new QMenuBar(this);
    //setMenuBar(menuBar);
    //QMenu *editMenu = menuBar->addMenu("Options");
    //QMenu *helpMenu = menuBar->addMenu("Help");

    // COLONNE A GAUCHE

    QWidget *leftPanel = new QWidget(m_mainWidget);
    QFormLayout *leftLayout = new QFormLayout(leftPanel);
    leftPanel->setObjectName("colonneGauche");

    m_suffix = new QLineEdit(leftPanel);
    m_suffix->setPlaceholderText("Tape ton texte ici");

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

    leftLayout->addRow("Suffix :", m_suffix);
    leftLayout->addRow("Model :", m_model);
    leftLayout->addRow("Open viewer :", m_toggleView);
    leftLayout->addRow("Output MNI space :", m_toggleOutput);
    leftLayout->addRow("Mode :", m_mode);


    // PARTIE PRINCIPALE A DROITE

    QWidget *mainArea = new QWidget(m_mainWidget);
    QVBoxLayout *mainAreaLayout = new QVBoxLayout(mainArea);
    mainArea->setObjectName("mainArea");

    m_fileButton = new QToolButton(mainArea);
    m_fileButton->setObjectName("chooseFileButton");
    m_fileButton->setText("Choisir un fichier");
    QIcon files_icon("../../../gui/ressources/files.png");
    m_fileButton->setIcon(files_icon);
    m_fileButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    m_runButton = new QPushButton("Run", mainArea);

    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(m_fileButton, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(m_runButton, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();

    connect(m_fileButton, &QPushButton::clicked, this, &MainWindow::chooseFile);

    mainLayout->addWidget(leftPanel);
    mainLayout->addWidget(mainArea);
    mainLayout->setStretch(0, 4);
    mainLayout->setStretch(1, 7);

    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    leftLayout->setSpacing(0);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    mainAreaLayout->setSpacing(0);
    mainAreaLayout->setContentsMargins(0, 0, 0, 0);

}

MainWindow::~MainWindow() {}

void MainWindow::chooseFile() {
    QString filename = QFileDialog::getOpenFileName(this, "Choose file");
    if (!filename.isEmpty()) {
        m_suffix->setText(filename); 
    }
}
