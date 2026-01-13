#include "InferenceEngine.h"

//#include <onnxruntime_cxx_api.h>
#include <winml/onnxruntime_cxx_api.h>
//#include <C:\\Users\\dvail\\source\\repos\\strokeseg2-app\\out\\build\\x64-Debug\\__nuget\\Microsoft.WindowsAppSDK.ML.1.8.2109\\include\\winml\\onnxruntime_cxx_api.h >
//#include <C:\\Users\\dvail\\source\\repos\\strokeseg2-app\\out\\build\\x64-Debug\\__nuget\\Microsoft.WindowsAppSDK.ML.1.8.2109\\include\\winml\\onnxruntime_c_api.h >
#include <winml/dml_provider_factory.h>
#include <utils/niftiVolume.h>

#include <iostream>
#include <algorithm>
#include <vector>

#include <QFile>

std::vector<float> InferenceEngine::RunInference(
    const QString &modelPath, 
    const QString &imagePath,
    const QString &inputName,
    const QString &outputName) 
    
    {

    if (!QFile::exists(modelPath)) {
        qDebug() << "Modèle introuvable !";
        return std::vector<float>();
    }

    if (!QFile::exists(imagePath)) {
        qDebug() << "Image introuvable !";
        return std::vector<float>();
    }
    
    // NIfTI to float vectors
    NiftiVolume nv = NiftiVolume::loadNifti(imagePath);

    int C = nv.data.dimension(0);
    int X = nv.data.dimension(1);
    int Y = nv.data.dimension(2);
    int Z = nv.data.dimension(3);

    std::vector<int64_t> inputShape = {1, C, X, Y, Z};
    std::vector<float> inputVector(
        nv.data.data(),
        nv.data.data() + nv.data.size()
    );
    
    // SESSION OPTIONS
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Inference");
    Ort::SessionOptions sessionOptions;
    
    
    auto providers = Ort::GetAvailableProviders();

    // CUDA
    if (std::find(providers.begin(), providers.end(), "CUDAExecutionProvider") != providers.end()) {
        std::cout << "Using CUDA Execution Provider\n";
        OrtStatus *status = OrtSessionOptionsAppendExecutionProvider_CUDA(sessionOptions, 0);
        if (status == nullptr) {
            std::cout << "CUDA activé avec succès\n";
        } else {
            std::cerr << "Erreur CUDA : " << Ort::GetApi().GetErrorMessage(status) << std::endl;
            Ort::GetApi().ReleaseStatus(status);
        }
    }
    // WindowsML / DirectML
    else if (std::find(providers.begin(), providers.end(), "DmlExecutionProvider") != providers.end()) {
        std::cout << "Using DirectML Execution Provider\n";
        OrtStatus *status = OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0);
        if (status == nullptr) {
            std::cout << "Using DirectML\n";
        } else {
            std::cerr << "Erreur DirectML : " << Ort::GetApi().GetErrorMessage(status) << std::endl;
            Ort::GetApi().ReleaseStatus(status);
        }
    } 
    // CPU
    else {
        std::cout << "Using CPU Execution Provider\n";
    }

    // CREATE SESSION
    Ort::Session session(
        env,
        modelPath.toStdWString().c_str(),
        sessionOptions
    );


    // TENSOR 
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, 
        const_cast<float *>(inputVector.data()), 
        inputVector.size(),
        inputShape.data(), inputShape.size()
    );
    
    // RUN
    std::string inputNameStr = inputName.toStdString();
    std::string outputNameStr = outputName.toStdString();
    const char *inputNames[] = {inputNameStr.c_str()};
    const char *outputNames[] = {outputNameStr.c_str()};
    
    try {
        std::vector<Ort::Value> outputTensors = session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

        // EXTRACT
        float *outputData = outputTensors[0].GetTensorMutableData<float>();
        size_t outputCount = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
        return std::vector<float>(outputData, outputData + outputCount);
    } catch (const Ort::Exception &e) {
        std::cerr << "Erreur lors de l'inférence : " << e.what() << std::endl;
        return {};
    }
}