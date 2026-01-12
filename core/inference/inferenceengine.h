#pragma once
#include <winrt/Windows.AI.MachineLearning.h>
#include <winml/onnxruntime_cxx_api.h>
#include <vector>
#include <filesystem>
#include <QString>

class InferenceEngine {

    InferenceEngine(const wchar_t *modelPath);

   
    /// <summary>
    /// Run inference and return output tensor
    /// </summary>
    static std::vector<float> RunInference(const QString &modelPath,
                                                const QString &imagePath,
                                                const QString& inputName = "input",
                                                const QString& outputName = "output"
    );

};