#include "viewer.h"

#include <QStandardPaths>
#include <QProcess>
#include <QDebug>

QString Viewer::itkSnapExe() const 
{
    #ifdef _WIN32
        return "itk-snap";
    #else
        return "itksnap";
    #endif

}

Viewer::Viewer(ConfigManager *config)
    : m_config(config), m_viewers(m_config->get("viewers", "itksnap").toStringList()) {
    const QString defaultViewer = m_config->get("viewer", "itksnap").toString();

	if (defaultViewer.isEmpty()) {
        UpdatePath(); 
    } else {
        // Check if there are new viewers
        QString path = m_config->get(defaultViewer, "").toString();
        if (path.isEmpty()) {
            UpdatePath();
        } else {
            // Create QStringList with empty viewers
            QStringList noPathViewers;
            for (const QString &v : m_viewers) {
                if (m_config->get(v, "").toString().isEmpty()) {
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
                    m_config->set(v, path);
                    m_config->save();
                }
            }
        }
    }
}

void Viewer::CheckViewers(const QString &viewer) 
{
    m_viewers = m_config->get("viewers", "").toString().split(',');
    for (const QString &v : m_viewers) {
        m_viewers.append(v);
    }
    if (!m_viewers.contains(viewer)) {
        throw std::runtime_error(QString("Viewer '%1' is not in the allowed list of viewers: %2")
                                     .arg(viewer)
                                     .arg(m_viewers.join(", "))
                                     .toStdString());
    }
    QString path = m_config->get(viewer, "").toString();
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
    m_config->set(viewer, path);
    m_config->set("viewer", viewer);
    m_config->save();
}

void Viewer::UpdatePath() 
{
    bool exist = false;
    for (const QString &v : m_viewers) {
        QString foundPath;
        if (v == "itksnap") {
            foundPath = QStandardPaths::findExecutable(itkSnapExe());
        } else {
            foundPath = QStandardPaths::findExecutable(v);
        }
        
        if (!foundPath.isEmpty()) {
            m_config->set(v, foundPath);
            m_config->set("viewer", v);
            exist = true;
        }
    }
    if (!exist) {
        m_config->set("viewer", "");
    }
    m_config->save();
}

void Viewer::Run(const QString &imgPath, const QString &segPath) {
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    const QString viewerName = m_config->get("viewer", "itksnap").toString();
    QString exePath = m_config->get(viewerName, "").toString();

    if (exePath.isEmpty()) {
        exePath = QStandardPaths::findExecutable(itkSnapExe());
    }

    if (exePath.isEmpty()) {
        qCritical() << "Impossible de trouver l'exécutable pour le viewer:" << viewerName;
        return;
    }
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    QStringList arguments;
    if (viewerName == "itksnap") {
        arguments << "-g" << QDir::toNativeSeparators(imgPath) << "-s"
                  << QDir::toNativeSeparators(segPath);
    } else {
        arguments << QDir::toNativeSeparators(imgPath) << QDir::toNativeSeparators(segPath);
    }

    bool success = QProcess::startDetached(exePath, arguments);

    QGuiApplication::restoreOverrideCursor();

    if (!success) {
        qCritical() << "Échec du lancement du viewer à l'emplacement :" << exePath;
    }
}

