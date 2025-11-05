#pragma once
#ifndef MAINWINDOW_H

#include <QMainWindow>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    MainWindow(QWidget *parent=0);
    ~MainWindow();

private:
    QWidget* m_mainWidget;
};

#endif // MAINWINDOW_H
