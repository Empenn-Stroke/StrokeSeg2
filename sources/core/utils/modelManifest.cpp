// modelManifest.cpp
#include <utils/modelManifest.h>

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace ModelManifest 
{

    QList<Modality> loadModalities(const QString &onnxPath)
    {
        QList<Modality> modalities;
        bool manifestValid = false;

        QFileInfo onnxInfo(onnxPath);
        QString manifestPath = onnxInfo.absolutePath() + "/" + onnxInfo.baseName() + ".json";
        QFile manifestFile(manifestPath);

        if (manifestFile.exists() && manifestFile.open(QIODevice::ReadOnly)) 
        {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(manifestFile.readAll(), &parseError);
            manifestFile.close();

            if (parseError.error == QJsonParseError::NoError && doc.isObject())
            {
                QJsonArray inputsArray = doc.object().value("inputs").toArray();
                for (const QJsonValue &val : inputsArray)
                {
                    if (val.isObject()) 
                    {
                        QJsonObject obj = val.toObject();
                        QString name = obj.value("name").toString().trimmed().toUpper();
                        QString refStr = obj.value("reference").toString().trimmed().toUpper();

                        if (!name.isEmpty())
                        {
                            Modality m;
                            m.name = name;
                            m.reference = referenceFamilyFromString(refStr.isEmpty() ? "T1" : refStr);
                            modalities << m;
                        }
                    }
                }
                manifestValid = !modalities.isEmpty();
            }
            else 
            {
                qWarning() << "[ModelManifest] JSON invalide dans" << manifestPath << ":" << parseError.errorString();
            }
        }
        else
        {
            qWarning() << "[ModelManifest] Aucun manifest trouvé pour" << onnxPath << "(" << manifestPath << ")";
        }

        if (!manifestValid)
        {
            modalities.clear();
            modalities << Modality{"T1", ReferenceFamily::T1};
            qWarning() << "[ModelManifest] Repli sur une entrée T1 unique pour" << onnxPath;
        }

        qDebug() << "[ModelManifest] Modalités chargées :" << modalities.size();
        for (const auto &m : modalities) {
            qDebug() << " -" << m.name << "(Ref:" << static_cast<int>(m.reference) << ")";
        }

        return modalities;
    }

}; // namespace ModelManifest
