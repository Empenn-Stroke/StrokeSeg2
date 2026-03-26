#pragma once
#include <dml_provider_factory.h>
#include <onnxruntime_cxx_api.h>
#include <QDir>
#include <QString>
#include <QStringList>
#include <utils/niftiVolume.h>
#include <utils/path.h>
#include <vector>

class InferenceEngine 
{

public:
    InferenceEngine();

    NiftiVolume run(const QString &modelPath, const QString &imagePath,
                    const QString &destinationPath, const QString &inputName,
                    const QString &outputName);


private:
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;

private:
    bool isDiscreteGPUPresent();

    QStringList getAvailableModels();
    
    bool loadModel(const QString &modelName);

    void initSession(const QString &modelPath);

    static std::vector<std::vector<int>> sliceVolume(std::array<int, 3> image_size,
                                              std::array<int, 3> patch_size, float step_size);

    static Eigen::Tensor<float, 3, Eigen::ColMajor> compute_gaussian(const std::array<int, 3> &patch_size,
                                                              float sigma_scale = 0.125f,
                                                              float value_scaling_factor = 10.0f);

    static QString getModelsPath() { return model_dir; }
};