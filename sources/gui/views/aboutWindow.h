// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <QFile>
#include <QTextEdit>

class AboutWindow : public QWidget {
    Q_OBJECT

  public:
    explicit AboutWindow(QWidget *parent = nullptr);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    bool m_dragging = false;
    QPoint m_dragPosition;

  private:
    QWidget *m_titleBar;
};