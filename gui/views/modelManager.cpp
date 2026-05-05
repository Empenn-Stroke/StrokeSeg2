#include "modelManager.h"

#include <QDir>
#include <QToolButton>
#include <QFileDialog>

#include <utils/env_path.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ModelManager::ModelManager(QWidget *parent) : QWidget(parent) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("Model manager");
    resize(250, 100);

    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);

    // =========================================================
    //                  GLOBAL STRUCTURE
    // =========================================================

    this->setObjectName("modelManager");

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

    QLabel *title = new QLabel("Model manager", m_titleBar);
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
    //                      MAIN AREA
    // =========================================================

    m_mainArea = new QWidget(this);
    m_mainArea->setObjectName("mainArea");
    m_mainArea->setAttribute(Qt::WA_StyledBackground, true);

    m_mainAreaLayout = new QVBoxLayout(m_mainArea);
    m_mainAreaLayout->setContentsMargins(12, 13, 12, 12);
    m_mainAreaLayout->setSpacing(8);

<<<<<<<< HEAD:gui/models/modelManager.cpp
    QDir modelsDir = QDir("C:/ProgramData/StrokeSeg/Model");
    QStringList entries = modelsDir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
========
    QDir modelsDir = Paths::modelDir();
    QStringList entries = modelsDir.entryList({"*.onnx"}, QDir::Files | QDir::NoDotAndDotDot);
>>>>>>>> dev:gui/views/modelManager.cpp

    for (const QString &entry : entries) {

        QString fullPath = modelsDir.filePath(entry);
        QFileInfo info(fullPath);
        QString displayName = info.baseName();

        QLabel *modelLabel = new QLabel(displayName, this);
        modelLabel->setObjectName("modelLabel");
        modelLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QWidget *modelContainer = new QWidget(m_mainArea);
        modelContainer->setContentsMargins(0, 0, 0, 0);
        QHBoxLayout *modelLayout = new QHBoxLayout(modelContainer);
        modelLayout->setContentsMargins(0, 0, 0, 0);
        modelLayout->setSpacing(8);

        QPushButton *deleteBtn = new QPushButton(modelContainer);
        deleteBtn->setObjectName("deleteBtn");
        QIcon *icon = new QIcon(":/gui/resources/delete.svg");
        deleteBtn->setIcon(*icon);
        deleteBtn->setIconSize(QSize(16, 16));

        connect(deleteBtn, &QPushButton::clicked, this, [this, fullPath, modelContainer]() {
            this->deleteModel(fullPath, modelContainer);
        });

        modelLayout->addWidget(modelLabel);
        modelLayout->addWidget(deleteBtn);

        m_mainAreaLayout->addWidget(modelContainer);
    }

    QPushButton *importModel = new QPushButton(m_mainArea);
    importModel->setText("+");
    importModel->setObjectName("importModel");
    importModel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    importModel->setContentsMargins(0, 10, 0, 0);

    m_mainAreaLayout->addStretch();
    m_mainAreaLayout->addWidget(importModel, 0, Qt::AlignHCenter);

    // =========================================================
    //                    FINAL ASSEMBLY
    // =========================================================

    windowLayout->addWidget(m_titleBar);
    windowLayout->addWidget(m_mainArea);

    // =========================================================
    //                     CONNECT
    // =========================================================

    connect(closeBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(reduceBtn, &QPushButton::clicked, this, &QWidget::showMinimized);
    connect(importModel, &QPushButton::clicked, this, &ModelManager::importModel);
}

// =========================================================
//                        METHODS
// =========================================================

bool ModelManager::eventFilter(QObject *obj, QEvent *event) {
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
            this->update();
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

bool ModelManager::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_WINDOWPOSCHANGED) {
        this->update(); // Redessine quand la position change au niveau système
    }
    return QWidget::nativeEvent(eventType, message, result);
}

void ModelManager::importModel() {
<<<<<<<< HEAD:gui/models/modelManager.cpp
    QString filename =
        QFileDialog::getOpenFileName(this, "Choose file", "C:/ProgramData/StrokeSeg/Model", "ONNX Model (*.onnx);;All files (*)");
========
    QString filename = QFileDialog::getOpenFileName(this, "Choose file", Paths::modelDir().absolutePath(),
                                                    "ONNX Model (*.onnx);;All files (*)");
>>>>>>>> dev:gui/views/modelManager.cpp

    if (filename.isEmpty())
        return;

    // ---- Find the path ----
<<<<<<<< HEAD:gui/models/modelManager.cpp
    QString programDataPath = qgetenv("PROGRAMDATA");
    if (programDataPath.isEmpty()) {
        programDataPath = "C:/ProgramData"; // Fallback manuel si la variable est vide
    }
    QDir dir(programDataPath + "/StrokeSeg/Model");
========
    QDir dir(Paths::modelDir());
>>>>>>>> dev:gui/views/modelManager.cpp

    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << ("Critical Error: Failed to create directory in ProgramData.");
            return;
        }
    }

    QString destFile = dir.filePath(QFileInfo(filename).fileName());

    // ---- Clear if already exists ----
    if (QFile::exists(destFile)) {
        if (!QFile::remove(destFile)) {
            qDebug() << ("Error: Unable to overwrite existing file (Access denied).");
            return;
        }
    }

    bool success = false;
    QString methodUsed = "";

    // ---- Try to hardlink ----
#ifdef Q_OS_WIN
    std::wstring src = filename.toStdWString();
    std::wstring dst = destFile.toStdWString();

    if (CreateHardLinkW(dst.c_str(), src.c_str(), NULL)) {
        success = true;
        methodUsed = "Hardlink";
    } else
        qDebug() << "Hardlink has failed (Probably not the same disk). Trying to copy...";
#endif

    // ---- Standard copy if the hardlink failed ----
    if (!success) {
        if (QFile::copy(filename, destFile)) {
            success = true;
            methodUsed = "Standard copy";
        }
    }

    // ---- Result ----
    if (success) {
        emit modelsChanged();
        qDebug() << "Success ! " << methodUsed << " created at : " << destFile;
    } else {
        qDebug() << "Total failure. Do verify admin access.";
    }

    QFileInfo info(destFile);
    QString displayName = info.baseName();

    QLabel *modelLabel = new QLabel(displayName, this);
    modelLabel->setObjectName("modelLabel");
    modelLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QWidget *modelContainer = new QWidget(m_mainArea);
    modelContainer->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout *modelLayout = new QHBoxLayout(modelContainer);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelLayout->setSpacing(8);

    QPushButton *deleteBtn = new QPushButton(modelContainer);
    deleteBtn->setObjectName("deleteBtn");
    QIcon *icon = new QIcon(":/gui/resources/delete.svg");
    deleteBtn->setIcon(*icon);
    deleteBtn->setIconSize(QSize(16, 16));

    connect(deleteBtn, &QPushButton::clicked, this,
            [this, destFile, modelContainer]() { this->deleteModel(destFile, modelContainer); });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(deleteBtn);

    int index = m_mainAreaLayout->count() - 2;
    m_mainAreaLayout->insertWidget(index < 0 ? 0 : index, modelContainer);
}

void ModelManager::deleteModel(const QString &filePath, QWidget *container) {
    if (QFile::exists(filePath)) {
        if (QFile::remove(filePath)) {
            emit modelsChanged();
            qDebug() << "Model successfully removed :" << filePath;
        } else {
            qDebug() << "Error : Unable to delete the model. Do verify admin access.";
            return; 
        }
    }

    if (container) {
        container->hide();
        container->deleteLater();
    }
}

