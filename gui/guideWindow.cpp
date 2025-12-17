#include "guideWindow.h"

GuideWindow::GuideWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Guide");
    resize(500, 300);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    this->setObjectName("guideWindow");


    QVBoxLayout *windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);

    // =========================================================
    //                      TITLE BAR
    // =========================================================

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(25);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(60, 0, 0, 2);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("Guide", m_titleBar);
    QPushButton *reduceBtn = new QPushButton("\u2212", m_titleBar);
    QPushButton *closeBtn = new QPushButton("\u00D7", m_titleBar);
    closeBtn->setObjectName("closeBtn");

    reduceBtn->setFixedSize(30, 25);
    closeBtn->setFixedSize(30, 25);

    titleLayout->addStretch();
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    titleLayout->addWidget(reduceBtn);
    titleLayout->addWidget(closeBtn);

    // =========================================================
    //                    MAIN AREA
    // =========================================================

    QWidget *mainArea = new QWidget(this);
    mainArea->setObjectName("guideContent");
    mainArea->setAttribute(Qt::WA_StyledBackground, true);

    // =========================================================
    //                    FINAL ASSEMBLY
    // =========================================================

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(mainArea);

    // =========================================================
    //                     CONNECT
    // =========================================================

    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
}

// =========================================================
//                        METHODS
// =========================================================

bool GuideWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == m_titleBar) {
        auto *e = static_cast<QMouseEvent *>(event);
        if (event->type() == QEvent::MouseButtonPress && e->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
            return true;
        }
        if (event->type() == QEvent::MouseMove && m_dragging) {
            move(e->globalPosition().toPoint() - m_dragPosition);
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            m_dragging = false;
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

