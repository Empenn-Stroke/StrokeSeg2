#include "mainWindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) 
{
	setWindowTitle("StrokeSeg2");

    QIcon icon("../../../gui/ressources/StrokeSeg2.ico");
    setWindowIcon(icon);

    m_mainWidget = new QWidget(this);

    setCentralWidget(m_mainWidget);

    resize(900,600);
}

MainWindow::~MainWindow() {}