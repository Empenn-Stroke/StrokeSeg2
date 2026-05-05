#include "warningWindow.h"

WarningWindow::WarningWindow(QDialog *parent) : QDialog(parent) {
    setWindowTitle("Warning");
    resize(400, 100);

    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    this->setObjectName("warningWindow");

    QVBoxLayout *windowLayout = new QVBoxLayout(this);
    windowLayout->setContentsMargins(0, 0, 0, 0);
    windowLayout->setSpacing(0);
    
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(30);
    shadow->setOffset(0, 0);
    shadow->setColor(QColor(25, 60, 105, 30));
    this->setGraphicsEffect(shadow);

    // =========================================================
    //                      TITLE BAR
    // =========================================================

    m_titleBar = new QWidget(this);
    m_titleBar->setObjectName("titleBar");
    m_titleBar->setFixedHeight(25);
    m_titleBar->installEventFilter(this);
    m_titleBar->setAttribute(Qt::WA_StyledBackground, true);

    QHBoxLayout *titleLayout = new QHBoxLayout(m_titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel("Warning", m_titleBar);

    //reduceBtn->setFixedSize(30, 25);

    titleLayout->addStretch();
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    // =========================================================
    //                    MAIN AREA
    // =========================================================

    // Main
    QWidget *mainArea = new QWidget(this);
    mainArea->setObjectName("mainArea");
    mainArea->setAttribute(Qt::WA_StyledBackground, true);

    QVBoxLayout *mainAreaLayout = new QVBoxLayout(mainArea);
    mainAreaLayout->setContentsMargins(0, 0, 0, 0);
    mainAreaLayout->setSpacing(0);

    // Text
    QTextEdit *textWarning = new QTextEdit(this);
    textWarning->setObjectName("warningContent");
    textWarning->setText("This application is for research purpose only !");
    textWarning->setReadOnly(true);
    textWarning->setAlignment(Qt::AlignCenter);
    textWarning->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textWarning->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textWarning->setContentsMargins(0, 0, 0, 0);



    // Dont show this again checkbox
    QWidget *dontShowWidget = new QWidget(this);

    QHBoxLayout *dontShowLayout = new QHBoxLayout(dontShowWidget);
    dontShowLayout->setContentsMargins(0, 0, 0, 0);
    dontShowLayout->setSpacing(0);

    QLabel *dontShowLabel = new QLabel("Do not show this again :", dontShowWidget);
    dontShowLabel->setObjectName("dontShowLabel");
    m_dontShowAgain = new QCheckBox(dontShowWidget);
    m_dontShowAgain->setObjectName("dontShowAgain");

    dontShowLayout->addStretch();
    dontShowLayout->addWidget(dontShowLabel);
    dontShowLayout->addWidget(m_dontShowAgain);
    dontShowLayout->addStretch();

    // Close button
    QPushButton *closeBtn = new QPushButton("I undertsand", m_titleBar);
    closeBtn->setObjectName("closeBtn");

    // Assembly
    mainAreaLayout->addStretch();
    mainAreaLayout->addWidget(textWarning);
    mainAreaLayout->addWidget(dontShowWidget);
    mainAreaLayout->addWidget(closeBtn, 0, Qt::AlignHCenter);
    mainAreaLayout->addStretch();

    // =========================================================
    //                    FINAL ASSEMBLY
    // =========================================================

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(mainArea);

    // =========================================================
    //                     CONNECT
    // =========================================================

    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);

    connect(m_dontShowAgain, &QCheckBox::toggled, this, [](bool checked) {
        QSettings settings;
        settings.setValue("showWarning", !checked);
    });
}

// =========================================================
//                        METHODS
// =========================================================

bool WarningWindow::eventFilter(QObject *obj, QEvent *event) {
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
    return QDialog::eventFilter(obj, event);
}

