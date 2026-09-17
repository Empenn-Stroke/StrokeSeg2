// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QString>

/**
 * @brief A utility class for converting DICOM files.
 *
 * This class provides static methods to check if a DICOM file requires conversion
 * and to perform the conversion process.
 */
namespace DicomConverter {
    /**
     * @brief Checks if a DICOM file requires conversion.
     *
     * @param path The path to the DICOM file to check.
     * @return True if the file requires conversion, false otherwise.
     */
     bool requiresConversion(const QString &path);

    /**
     * @brief Converts a DICOM file to a specified output directory.
     *
     * @param inputPath The path to the input DICOM file.
     * @param outputDir The directory where the converted file should be saved.
     * @return The path to the converted file.
     */
    QString convert(const QString &inputPath, const QString &outputDir);
} // namespace DicomConverter
