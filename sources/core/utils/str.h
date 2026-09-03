#pragma once

#include <QString>
#include <array>
#include <utility>
#include <format>

/*
 * TODO : update the different texts to fit the new version and the new team members
 */


static QString developers = R"DEV(Alessandro Di Matteo (Department of Information Engineering, Computer Science and Mathematics, University of L'Aquila)
Youwan Mahé (Neuroimaging : Methods and Applications, Siemens Healthineers)
Stéphanie Leplaideur (Physical Medicine and Rehabilitation, Clinical Investigation Center, Rennes University Hospital)
Florent Leray (Inria / IRISA – Beaulieu University Campus)
Elise Bannier (Neuroimaging : Methods and Applications, Radiology Department, Rennes University Hospital)
Francesca Galassi (Neuroimaging : Methods and Applications)
Yann Kerverdo (Inria – Beaulieu University Campus)
Mathilde Liffran (Graphics & Logos)
)DEV";

static QString app_name = "StrokeSeg";
static QString version = "2.0";

static QString license = QString(R"LIC(%1 %2
INRIA, University of Rennes, France
Email : strokeseg@inria.fr
AGPL v3 license

This application uses the following open-source Python libraries:
- ONNX Runtime, 1.22.0, github.com/microsoft/onnxruntime
- nibabel, 5.3.2, github.com/nipy/nibabel
- scipy, 1.16.0, github.com/scipy/scipy

This application also uses Anima for medical image processing which is distributed under the AGPL v3 license, github.com/Inria-Empenn/Anima-Public
)LIC").arg(app_name, version);

// Publications as a constexpr array of (title, authors/citation)
static std::array<std::pair<QString, QString>, 1> publications = {
    {
    { "Deep Learning and Multi-Modal MRI for the Segmentation of Sub-Acute and Chronic Stroke Lesions",
      "Alessandro Di Matteo, Youwan Mahé, Stéphanie Leplaideur, Isabelle Bonan, Elise Bannier, et al. (2025) ⟨hal-04647365v2⟩" }
}};

// Utility to produce a human-readable application header (runtime string)
inline QString ApplicationHeader() {
    QString out;
    out.reserve(512);
    out += app_name;
    out += " v";
    out += version;
    out += "\n\nDevelopers:\n";
    out += developers;
    out += "\nLicense:\n";
    out += license;
    out += "\n\nPublications:\n";
    for (auto const &p : publications) {
        out += "- ";
        out += p.first;
        out += " — ";
        out += p.second;
        out += "\n";
    }
    return out;
}
