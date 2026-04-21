#pragma once

#include <QString>

class DicomConverter {
  public:
    static bool requiresConversion(const QString &path);

    static QString convert(const QString &inputPath, const QString &outputDir);
};