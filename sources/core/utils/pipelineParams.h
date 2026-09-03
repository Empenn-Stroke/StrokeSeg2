// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QMap>

#include <utils/modality.h>

/**
 * @brief A structure to hold parameters for the StrokeSeg2 pipeline. It includes paths for input files, model files, output directories, and various flags to control the behavior of the pipeline. 
 */
struct PipelineParams 
{
    QMap<QString, QString> inputPaths;
    QStringList inputOrder;
    QList<Modality> modalities;
    QString modelPath;
    QString outputDir;
    QString suffix;
    float threshold;
    bool openFolder;
    bool openViewer;
    bool savePMap;
    bool saveInterSteps;
    bool skipPreProcessing;
    bool skipInference;
    bool skipPostProcessing;
    bool brainExtraction = true;
    bool gui;
    bool mni;
    bool betOnly;



    /**
     * @brief Validates the pipeline parameters to ensure that required fields are set and files exist.
     * @param errorMessage A reference to a QString that will hold the error message if validation fails.
     * @return True if the parameters are valid, false otherwise.
     */
    bool isValid(const QStringList &requiredInputs, QString &error) const {
        for (const QString &input : requiredInputs) {
            QString key = input.toUpper();
            if (!inputPaths.contains(key) || inputPaths[key].isEmpty()) {
                error = QString("Missing required input: %1").arg(key);
                return false;
            }
        }
        return true && !outputDir.isEmpty();
    }
};
