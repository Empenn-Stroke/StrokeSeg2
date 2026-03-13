#include "inferenceengine.h"
#include <QDebug>
#include <QFile>
#include <algorithm>
#include <iostream>
#include <vector>

using OrtFloat16 = Ort::Float16_t;

InferenceEngine::InferenceEngine() {}

QStringList InferenceEngine::getAvailableModels() {
    QString programDataPath = qgetenv("PROGRAMDATA");
    if (programDataPath.isEmpty())
        programDataPath = "C:/ProgramData";

    QDir modelsDir(programDataPath + "/StrokeSeg/Models");
    return modelsDir.entryList({"*.onnx"}, QDir::Files);
}

void InferenceEngine::loadModel(const QString &modelName) {

    QString fullPath = getModelsPath() + modelName;

    if (QFile::exists(fullPath)) {
        this->initSession(fullPath);
    }
}

void InferenceEngine::initSession(const QString &modelPath) {
    env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "Inference");
    Ort::SessionOptions sessionOptions;

    auto providers = Ort::GetAvailableProviders();
    if (std::find(providers.begin(), providers.end(), "DmlExecutionProvider") != providers.end()) {
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0));
        sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
    }

    session =
        std::make_unique<Ort::Session>(*env, modelPath.toStdWString().c_str(), sessionOptions);
}

NiftiVolume InferenceEngine::runInference(const QString &modelPath, 
                                          const QString &imagePath,
                                          const QString &destinationPath,
                                          const QString &inputName,
                                          const QString &outputName) {

    if (!QFile::exists(modelPath) || !QFile::exists(imagePath)) {
        qDebug() << "Image or model not found !";
        return {};
    }

    // ---- Load and read Nifti ----

    NiftiVolume nv = NiftiVolume::loadNifti(imagePath);

    int C = nv.data.dimension(0);
    int X = nv.data.dimension(1);
    int Y = nv.data.dimension(2);
    int Z = nv.data.dimension(3);

    std::vector<int64_t> inputShape = {1, C, X, Y, Z};
    size_t size = C * X * Y * Z;

    // ---- Convert to float 16 for ONNX ----

    auto casted_expression = nv.data.cast<OrtFloat16>();

    std::vector<OrtFloat16> inputVector(nv.data.size());

    Eigen::TensorMap<Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor>> map(
        reinterpret_cast<OrtFloat16 *>(inputVector.data()), C, X, Y, Z);

    map = casted_expression; 

    // ---- Set environment ----
    initSession(modelPath);

    
    // ---- Tensor set ----
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<OrtFloat16>(
        memoryInfo, inputVector.data(), inputVector.size(), inputShape.data(), inputShape.size());

    std::string inputNameStr = inputName.toStdString();
    std::string outputNameStr = outputName.toStdString();
    const char *inputNames[] = {inputNameStr.c_str()};
    const char *outputNames[] = {outputNameStr.c_str()};

    
    // ---- Session run and extract ----
    try {
        std::vector<Ort::Value> outputTensors =
            session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
            
        OrtFloat16 *outputDataRaw = outputTensors[0].GetTensorMutableData<OrtFloat16>();
        size_t outputCount = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();

        std::vector<float> result(outputCount);
        for (size_t i = 0; i < outputCount; ++i) {
            result[i] = outputDataRaw[i].ToFloat();
        }

        NiftiVolume outputVol = nv; // Copy spacing and metadata
        outputVol.spacing = nv.spacing;
        outputVol.data = NiftiVolume::Tensor4f(C, X, Y, Z);

        qDebug() << "Max value in result:" << *std::max_element(result.begin(), result.end());

        Eigen::TensorMap<Eigen::Tensor<float, 4, Eigen::ColMajor>> result_tensor(result.data(), C,
                                                                                 X, Y, Z);

        outputVol.data =
            (result_tensor.isnan()).select(result_tensor.constant(0.0f), result_tensor);

        NiftiVolume::saveNifti(destinationPath, outputVol);

        return outputVol;
    } catch (const Ort::Exception &e) {
        std::cerr << "Error during inference : " << e.what() << std::endl;
        return {};
    }

}