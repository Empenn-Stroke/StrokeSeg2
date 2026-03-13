#pragma once
#include <vector>
#include <QString>
#include <utils/niftiVolume.h>
#include <QDir>
#include <QStringList>
#include <dml_provider_factory.h>
#include <onnxruntime_cxx_api.h>

class InferenceEngine {

private:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;

public:

    InferenceEngine();

    QStringList getAvailableModels();

    void loadModel(const QString &modelName);

    void initSession(const QString &modelPath);

    std::array<std::vector<int>, 3>
    sliceVolume(
        NiftiVolume &vol, 
        int patches_number
    );
   
    /// <summary>
    /// Run inference and return output tensor
    /// </summary>
    NiftiVolume runInference(
        const QString &modelPath,
        const QString &imagePath,
        const QString &destinationPath,
        const QString& inputName = "input",
        const QString& outputName = "output"
    );

    static QString getModelsPath() { return qgetenv("PROGRAMDATA") + "/StrokeSeg/Models"; }

}; //