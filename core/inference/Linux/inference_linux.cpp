#include "inference_linux.h"
#include "../inference.h"

// Allocation de la classe privée stubbée
Inference::Inference() : d(std::make_unique<InferencePrivate>()) {}

Inference::~Inference() = default;

// Stub pour le chargement du modèle
bool Inference::loadModel(const QString& modelPath) {
    (void)modelPath;
    return true;
}