// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QString>
#include <QList>

/**
 * @brief Enum class for reference families.
 *
 * This enum class represents different reference families used in medical imaging modalities.
 */
enum class ReferenceFamily { T1, T2 };

/**
 * @brief Converts a ReferenceFamily enum to a QString.
 *
 * @param f The ReferenceFamily enum to convert.
 * @return A QString representing the reference family.
 */
inline QString referenceFamilyToString(ReferenceFamily f) { return (f == ReferenceFamily::T2) ? "T2" : "T1"; }

/**
 * @brief Converts a QString to a ReferenceFamily enum.
 *
 * @param s The QString to convert.
 * @return The corresponding ReferenceFamily enum.
 */
inline ReferenceFamily referenceFamilyFromString(const QString &s) { return (s.trimmed().toUpper() == "T2") ? ReferenceFamily::T2 : ReferenceFamily::T1; }

/**
 * @brief Struct representing a modality.
 *
 * This struct holds information about a medical imaging modality, including its name
 * and the reference family it aligns to.
 */
struct Modality 
{
    QString name;                 // e.g. "T1", "T1C", "FLAIR", "T2"
    ReferenceFamily reference;    // which MNI atlas family this modality aligns to
};
