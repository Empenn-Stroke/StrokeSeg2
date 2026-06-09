#pragma once

#include <QString>
#include <onnxruntime_cxx_api.h>

class InferencePrivate {
public:
    // Un pointeur nul suffit pour la compilation
    Ort::Session* m_session = nullptr;

    // Stub de la fonction d'initialisation
    void init(const QString& modelPath) {
        (void)modelPath; // Évite le warning "unused parameter"
    }
};