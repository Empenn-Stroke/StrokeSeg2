#pragma once
#ifndef MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QLineEdit>
#include <QCheckbox>
#include <QPushButton>
#include <QToolButton>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    MainWindow(QWidget *parent=0);
    ~MainWindow();

private:
    QWidget* m_mainWidget;

    QLineEdit *m_suffix;
    QComboBox *m_model;
    QCheckBox *m_toggleView;
    QCheckBox *m_toggleOutput;
    QComboBox *m_mode;

    QToolButton *m_fileButton;
    QPushButton *m_runButton;

  private slots:
    void chooseFile();
};


#endif // MAINWINDOW_H

