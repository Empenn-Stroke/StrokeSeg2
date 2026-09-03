// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialog>
#include <QWidget>

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>

#include <QTextEdit>
#include <QCheckBox>

#include <QSettings>

class WarningWindow : public QDialog {
    Q_OBJECT

  public:
    explicit WarningWindow(QDialog *parent = nullptr);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    bool m_dragging = false;
    QPoint m_dragPosition;

  private:
    QWidget *m_titleBar;
    QCheckBox *m_dontShowAgain = nullptr;
};