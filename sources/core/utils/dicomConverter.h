// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QString>

class DicomConverter {
  public:
    static bool requiresConversion(const QString &path);

    static QString convert(const QString &inputPath, const QString &outputDir);
};
