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

class ModelManager : public QWidget
{
    Q_OBJECT

  public:
    explicit ModelManager(QWidget *parent = nullptr);

  protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;

    bool m_dragging = false;
    QPoint m_dragPosition;

  private:
    QWidget *m_titleBar;
    QWidget *m_mainArea;
    QVBoxLayout *m_mainAreaLayout ;

  private:
    void addModelEntry(const QString &modelPath);
    QStringList promptForChannelNames(bool &ok);
    bool writeModelManifest(const QString &onnxPath, const QStringList &inputs, QString &errorMessage);
    
  private slots:
    void importModel();
    void deleteModel(const QString &filePath, QWidget *container);
  
  signals:
    void modelsChanged();
};