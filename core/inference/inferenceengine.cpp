#include "InferenceEngine.h"
#include <utils/niftiVolume.h>

std::vector<float> InferenceEngine::RunInference(
    const QString &modelPath, 
    const QString &imagePath,
    const QString &inputName = "input",
    const QString &outputName = "output") 
    
    {
    
    // NIfTI to float vectors
    NiftiVolume nv = NiftiVolume::loadNifti(imagePath);

    int C = nv.data.dimension(0);
    int X = nv.data.dimension(1);
    int Y = nv.data.dimension(2);
    int Z = nv.data.dimension(3);

    std::vector<int64_t> inputShape = {1, C, Z, Y, X};
    std::vector<float> inputVector = std::vector<float>(nv.data.data(),
                                                        nv.data.data() + nv.data.size());
    
    // SESSION OPTIONS
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Inference");
    Ort::SessionOptions sessionOptions;
    
    // CUDA
    if (Ort::GetAvailableProviders().count("CUDAExecutionProvider")) {
        sessionOptions.AppendExecutionProvider_CUDA(0);
    }
    // WindowsML / DirectML
    else if (Ort::GetAvailableProviders().count("DmlExecutionProvider")) {
        sessionOptions.AppendExecutionProvider_DML(0);
    } else {
        std::cout << "Using CPU (default)\n";
    }

    // CREATE SESSION
    Ort::Session session(env, modelPath.toStdString().c_str(), sessionOptions);


    // TENSOR 
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, 
        const_cast<float *>(inputVector.data()), 
        inputVector.size(),
        inputShape.data(), inputShape.size()
    );
    
    // RUN
    const char *inputNames[] = {inputName.toStdString().c_str()};
    const char *outputNames[] = {outputName.toStdString().c_str()};
    
    std::vector<Ort::Value> outputTensors = session.Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

    // EXTRACT
    float *outputData = outputTensors[0].GetTensorMutableData<float>();
    size_t outputCount = outputTensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
    std::vector<float> result(outputData, outputData + outputCount);

    return result;
}