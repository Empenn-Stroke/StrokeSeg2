#pragma once
#include <vector>
#include <QString>

class InferenceEngine {

public:
   
    /// <summary>
    /// Run inference and return output tensor
    /// </summary>
    static std::vector<float> RunInference(
        const QString &modelPath,
        const QString &imagePath,
        const QString& inputName = "input",
        const QString& outputName = "output"
    );

};