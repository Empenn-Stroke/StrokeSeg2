// modelManifest.h
#pragma once
#include "utils/modality.h"
#include <QList>
#include <QString>

namespace ModelManifest
{
    /**
     * @brief Loads the ordered list of modalities (name + reference family) from
     * <modelBaseName>.json, located next to the given .onnx model file.
     * Falls back to a single T1 modality if missing/invalid.
     */
    QList<Modality> loadModalities(const QString &onnxPath);
} // namespace ModelManifest
