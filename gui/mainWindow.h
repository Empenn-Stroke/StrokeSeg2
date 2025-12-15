#pragma once
#ifndef MAINWINDOW_H

#include <QMainWindow>
#include <QApplication>
#include <QLineEdit>
#include <QCheckbox>
#include <QPushButton>
#include <QToolButton>
#include <QSlider>

class MainWindow : public QMainWindow {
	Q_OBJECT

public:
    MainWindow(QWidget *parent=0);
    ~MainWindow();

    bool m_dragging = false;
    QPoint m_dragPosition;

private:
    QWidget *m_mainWidget;
    QWidget *m_titleBar;

    QLineEdit *m_suffix;
    QComboBox *m_model;
    QCheckBox *m_toggleView;
    QCheckBox *m_toggleOutput;
    QComboBox *m_mode;
    QSlider *m_threshold;

    QToolButton *m_fileButton;
    QPushButton *m_runButton;


  private slots:
    void chooseFile();
    bool eventFilter(QObject *obj, QEvent *event) override;
};


#endif // MAINWINDOW_H

