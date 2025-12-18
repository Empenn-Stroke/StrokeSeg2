#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

#include <QTextEdit>
#include <QFile>

class GuideWindow : public QWidget {
    Q_OBJECT
  
    public:
        explicit GuideWindow(QWidget *parent = nullptr);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

        bool m_dragging = false;
        QPoint m_dragPosition;

    private:
        QWidget *m_titleBar;
};