#pragma once
#include <winrt/Microsoft.Windows.AI.MachineLearning.h>
#include <winml/onnxruntime_cxx_api.h>
#include <vector>
#include <filesystem>
#include <QString>

class InferenceEngine {

    InferenceEngine(const wchar_t *modelPath);

   
    /// <summary>
    /// Run inference and return output tensor
    /// </summary>
    static std::vector<Ort::Value> RunInference(const QString& modelPath,
                                                const std::vector<float>& inputVector, 
                                                const std::vector<int64_t>& inputShape,
                                                const QString& inputName = "input",
                                                const QString& outputName = "output") 
    );

};