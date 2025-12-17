#pragma once
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>

#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

class GuideWindow : public QWidget {
    Q_OBJECT
  
    public:
        explicit GuideWindow(QWidget *parent = nullptr);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

        bool m_dragging = false;
        QPoint m_dragPosition;

    private:
        QWidget *m_mainWidget;
        QWidget *m_titleBar;
};