// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <QString>
#include <QList>

enum class ReferenceFamily { T1, T2 };

inline QString referenceFamilyToString(ReferenceFamily f) 
{
    return (f == ReferenceFamily::T2) ? "T2" : "T1";
}

inline ReferenceFamily referenceFamilyFromString(const QString &s)
{
    return (s.trimmed().toUpper() == "T2") ? ReferenceFamily::T2 : ReferenceFamily::T1;
}

struct Modality 
{
    QString name;                 // e.g. "T1", "T1C", "FLAIR", "T2"
    ReferenceFamily reference;    // which MNI atlas family this modality aligns to
};
