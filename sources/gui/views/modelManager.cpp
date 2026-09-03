// SPDX-License-Identifier: AGPL-3.0-or-later

#include "modelManager.h"

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QToolButton>
#include <QtGlobal>

#include <utils/env_path.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ModelManager::ModelManager(QWidget *parent) : QWidget(parent)
{
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

    QDir modelsDir = Paths::modelDir();
    QStringList entries = modelsDir.entryList({"*.onnx"}, QDir::Files | QDir::NoDotAndDotDot);

    for (const QString &entry : entries)
    {
        addModelEntry(modelsDir.filePath(entry));
    }

    QPushButton *importModelBtn = new QPushButton(m_mainArea);
    importModelBtn->setText("+");
    importModelBtn->setObjectName("importModel");
    importModelBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    importModelBtn->setContentsMargins(0, 10, 0, 0);

    m_mainAreaLayout->addStretch();
    m_mainAreaLayout->addWidget(importModelBtn, 0, Qt::AlignHCenter);

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
    connect(importModelBtn, &QPushButton::clicked, this, &ModelManager::importModel);
}

// =========================================================
//                        METHODS
// =========================================================

bool ModelManager::eventFilter(QObject *obj, QEvent *event)
{
    bool handled = false;

    if (obj == m_titleBar)
    {
        auto *e = static_cast<QMouseEvent *>(event);

        if (event->type() == QEvent::MouseButtonPress && e->button() == Qt::LeftButton)
        {
            m_dragging = true;
            m_dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
            handled = true;
        }
        else if (event->type() == QEvent::MouseMove && m_dragging)
        {
            move(e->globalPosition().toPoint() - m_dragPosition);
            handled = true;
        }
        else if (event->type() == QEvent::MouseButtonRelease)
        {
            m_dragging = false;
            this->update();
            handled = true;
        }
    }

    bool result = handled ? true : QWidget::eventFilter(obj, event);
    return result;
}

bool ModelManager::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
    MSG *msg = static_cast<MSG *>(message);
    if (msg->message == WM_WINDOWPOSCHANGED) {
    }
#elif defined(Q_OS_MAC)
#endif
    return QWidget::nativeEvent(eventType, message, result);
}

/**
 * @brief Creates and inserts a UI row (label + delete button) for a given model file.
 * Shared between the constructor (initial listing) and importModel() (new entries),
 * to avoid duplicating widget-construction code.
 * @param modelPath Full path to the .onnx file.
 */
void ModelManager::addModelEntry(const QString &modelPath)
{
    QFileInfo info(modelPath);
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

    connect(deleteBtn, &QPushButton::clicked, this, [this, modelPath, modelContainer]()
    {
        this->deleteModel(modelPath, modelContainer);
    });

    modelLayout->addWidget(modelLabel);
    modelLayout->addWidget(deleteBtn);

    int index = m_mainAreaLayout->count() - 2;
    m_mainAreaLayout->insertWidget(index < 0 ? 0 : index, modelContainer);
}

/**
 * @brief Prompts the user for a comma-separated, ordered list of input channel names
 * (e.g. "T1, FLAIR") and returns them normalized (trimmed, uppercased, empties removed).
 * @param ok Set to false if the user cancelled or entered no valid channel name.
 */
QStringList ModelManager::promptForChannelNames(bool &ok)
{
    QStringList channels;

    bool dialogAccepted = false;
    QString text = QInputDialog::getText(this, "Input channels", "Enter the required input channels for this model, in order, separated by commas\n(e.g. T1, FLAIR):", QLineEdit::Normal, "T1", &dialogAccepted);

    if (dialogAccepted)
    {
        const QStringList rawParts = text.split(",", Qt::SkipEmptyParts);
        for (const QString &part : rawParts)
        {
            QString cleaned = part.trimmed().toUpper();
            if (!cleaned.isEmpty())
            {
                channels << cleaned;
            }
        }
    }

    ok = dialogAccepted && !channels.isEmpty();
    return channels;
}

/**
 * @brief Writes a JSON manifest (<baseName>.json) next to the given .onnx model,
 * describing its ordered list of required input channels.
 * @param onnxPath Full path to the .onnx file.
 * @param inputs Ordered channel names (e.g. {"T1", "FLAIR"}).
 * @param errorMessage Set on failure.
 * @return True on success.
 */
bool ModelManager::writeModelManifest(const QString &onnxPath, const QStringList &inputs, QString &errorMessage)
{
    bool success = false;

    QFileInfo onnxInfo(onnxPath);
    QString manifestPath = onnxInfo.absolutePath() + "/" + onnxInfo.baseName() + ".json";

    QJsonArray inputsArray;
    for (const QString &channel : inputs)
    {
        inputsArray.append(channel);
    }

    QJsonObject root;
    root["name"] = onnxInfo.baseName();
    root["inputs"] = inputsArray;

    QFile manifestFile(manifestPath);
    if (manifestFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        QJsonDocument doc(root);
        qint64 written = manifestFile.write(doc.toJson(QJsonDocument::Indented));
        manifestFile.close();
        success = (written > 0);
        if (!success)
        {
            errorMessage = "Failed to write manifest content: " + manifestPath;
        }
    }
    else
    {
        errorMessage = "Failed to open manifest for writing: " + manifestPath;
    }

    return success;
}

void ModelManager::importModel()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, "Choose file", Paths::modelDir().absolutePath(), "ONNX Model (*.onnx);;All files (*)", nullptr, QFileDialog::DontUseNativeDialog);

    if (!filename.isEmpty())
    {
        bool channelsOk = false;
        QStringList channels = promptForChannelNames(channelsOk);

        if (!channelsOk)
        {
            QMessageBox::warning(this, "Import cancelled", "At least one input channel name is required to import a model.");
        }
        else
        {
            QDir dir(Paths::modelDir());
            bool dirReady = dir.exists();

            if (!dirReady)
            {
                dirReady = dir.mkpath(".");
                if (!dirReady)
                {
                    QMessageBox::critical(this, "Import failed", "Critical Error: Failed to create directory in ProgramData.");
                }
            }

            if (dirReady)
            {
                QString destFile = dir.filePath(QFileInfo(filename).fileName());

                bool destClear = true;
                if (QFile::exists(destFile))
                {
                    destClear = QFile::remove(destFile);
                    if (!destClear)
                    {
                        QMessageBox::critical(this, "Import failed", "Error: Unable to overwrite existing file (Access denied).");
                    }
                }

                if (destClear)
                {
                    bool copySuccess = false;
                    QString methodUsed = "";

#ifdef Q_OS_WIN
                    std::wstring src = filename.toStdWString();
                    std::wstring dst = destFile.toStdWString();

                    if (CreateHardLinkW(dst.c_str(), src.c_str(), NULL))
                    {
                        copySuccess = true;
                        methodUsed = "Hardlink";
                    }
                    else
                    {
                        qDebug() << "Hardlink has failed (Probably not the same disk). Trying to copy...";
                    }
#endif

                    if (!copySuccess) {
                        if (QFile::copy(filename, destFile))
                        {
                            copySuccess = true;
                            methodUsed = "Standard copy";
                        }
                    }

                    if (!copySuccess)
                    {
                        QMessageBox::critical(this, "Import failed", "Total failure while copying the model file. Please verify admin access.");
                    }
                    else
                    {
                        QString manifestError;
                        bool manifestSuccess = writeModelManifest(destFile, channels, manifestError);

                        if (!manifestSuccess)
                        {
                            // On retire le .onnx orphelin pour ne pas laisser un modèle sans manifest valide.
                            QFile::remove(destFile);
                            QMessageBox::critical(this, "Import failed", "Model file copied but manifest creation failed:\n" + manifestError);
                        }
                        else
                        {
                            qDebug() << "Success !" << methodUsed << "created at:" << destFile;
                            addModelEntry(destFile);
                            emit modelsChanged();
                        }
                    }
                }
            }
        }
    }
}

void ModelManager::deleteModel(const QString &filePath, QWidget *container)
{
    bool removalSuccess = true;

    if (QFile::exists(filePath))
    {
        removalSuccess = QFile::remove(filePath);

        if (removalSuccess)
        {
            QFileInfo info(filePath);
            QString manifestPath = info.absolutePath() + "/" + info.baseName() + ".json";
            if (QFile::exists(manifestPath))
            {
                bool manifestRemoved = QFile::remove(manifestPath);
                if (!manifestRemoved)
                {
                    qDebug() << "Warning: Failed to remove associated manifest:" << manifestPath;
                }
            }

            qDebug() << "Model successfully removed :" << filePath;
            emit modelsChanged();
        }
        else
        {
            QMessageBox::critical(this, "Deletion failed", "Error: Unable to delete the model. Please verify admin access.");
        }
    }

    if (removalSuccess && container)
    {
        container->hide();
        container->deleteLater();
    }
}
