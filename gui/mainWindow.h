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

    bool m_dragging = false;
    QPoint m_dragPosition;

private:
    QWidget *m_mainWidget;

    QLineEdit *m_suffix;
    QComboBox *m_model;
    QCheckBox *m_toggleView;
    QCheckBox *m_toggleOutput;
    QComboBox *m_mode;

    QToolButton *m_fileButton;
    QPushButton *m_runButton;


  private slots:
    void chooseFile();
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};


#endif // MAINWINDOW_H

