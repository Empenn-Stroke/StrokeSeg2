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

    std::vector<std::vector<int>>
    sliceVolume(std::array<int, 3> image_size,
                std::array<int, 3> patch_size, 
                float step_size);

    Eigen::Tensor<float, 3, Eigen::ColMajor>
    compute_gaussian(const std::array<int, 3> &patch_size,
                     float sigma_scale = 0.125f,
                     float value_scaling_factor = 10.0f);
   
    /// <summary>
    /// Run inference and return output tensor
    /// </summary>
    NiftiVolume run(const QString &modelPath, const QString &imagePath,
                    const QString &destinationPath, const QString &inputName,
                    const QString &outputName);

    static QString getModelsPath() { return qgetenv("PROGRAMDATA") + "/StrokeSeg/Models"; }

}; //