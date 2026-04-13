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

class ModelManager : public QWidget {
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
    
  private slots:
    void importModel();
    void deleteModel(const QString &filePath, QWidget *container);
  
  signals:
    void modelsChanged();
};