#include "viewer.h"

#include <QStandardPaths>
#include <QProcess>
#include <QDebug>

QString Viewer::itkSnapExe() const {
    #ifdef _WIN32
        return "itk-snap";
    #else
        return "itksnap";
    #endif

}

Viewer::Viewer() : m_viewers(m_config.get("viewers", "").split(',')) {

    const QString defaultViewer = m_config.get("viewer", "");

	if (defaultViewer.isEmpty()) {
        UpdatePath(); 
    } else {
        // Check if there are new viewers
        QString path = m_config.get(defaultViewer, "");
        if (path.isEmpty()) {
            UpdatePath();
        } else {
            // Create QStringList with empty viewers
            QStringList noPathViewers;
            for (const QString &v : m_viewers) {
                if (m_config.get(v, "").isEmpty()) {
                    noPathViewers.append(v);
                }
            }
            for (const QString &v : noPathViewers) {
                if (v == "itksnap") {
                    path = QStandardPaths::findExecutable(itkSnapExe()); // The correct shortcut on Windows is itk-snap, not itksnap
                } else {
                    path = QStandardPaths::findExecutable(v);
                }
                if (!path.isEmpty()) {
                    m_config.set(v, path);
                    m_config.save();
                }
            }
        }
    }
}

void Viewer::CheckViewers(const QString &viewer) {
    m_viewers = m_config.get("viewers", "").split(',');
    for (const QString &v : m_viewers) {
        m_viewers.append(v);
    }
    if (!viewers.contains(viewer)) {
        throw std::runtime_error(QString("Viewer '%1' is not in the allowed list of viewers: %2")
                                     .arg(viewer)
                                     .arg(viewers)
                                     .toStdString());
    }
    QString path = m_config.get(viewer, "");
    if (path.isEmpty()) {
        if (viewer == "itksnap") {
            path = QStandardPaths::findExecutable(itkSnapExe());
        } else {
            path = QStandardPaths::findExecutable(viewer);
        } 
        if (path.isEmpty()) {
            throw std::runtime_error(QString("Viewer '%1' not found in Path.")
                                            .arg(viewer)
                                            .toStdString());
        }
    }
    m_config.set(viewer, path);
    m_config.set("viewer", viewer);
    m_config.save();
}

void Viewer::UpdatePath() {
    bool exist = false;
    for (const QString &v : m_viewers) {
        if (v == "itksnap") {
            path = QStandardPaths::findExecutable(itkSnapExe());
        } else {
            path = QStandardPaths::findExecutable(v);
        }
        if (!path.isEmpty()) {
            m_config.set(v, path);
            m_conifg.set("viewer", v);
            exist = true;
        }
    }
    if (!exist) {
        m_config.set("viewer", "");
    }
    m_config.save();
} 

void Viewer::Run(const QString &imgPath, const QString &segPath) {

    const QString defaultViewer = m_config.get("viewer", "");
    QString path = m_config.get(viewer, "");
    QStringList = command;

    if (defaultViewer == "itksnap") {
        command.append(QStringList{path, "-g", imgPath, "-s", segPath});
    } else {
        command.append(QStringList{path, imgPath, segPath});
    }

    try :
        QProcess *process = new QProcess();
        process->setStandardOutputFile(QProcess::nullDevice()); // ignore stdout
        process->setStandardErrorFile(QProcess::nullDevice()); // ignore stderr
        process->start(defaultViewer, command);
    catch : 
        UpdatePath();
        qCritical() << QString("Failed to execute command %1: %2")
                                .arg(command)
                                .arg(error); 
}

