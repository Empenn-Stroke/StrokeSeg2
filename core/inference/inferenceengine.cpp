#include "inferenceengine.h"
#include <QDebug>
#include <QFile>
#include <algorithm>
#include <dml_provider_factory.h>
#include <iostream>
#include <onnxruntime_cxx_api.h>
#include <utils/niftiVolume.h>
#include <vector>

using OrtFloat16 = Ort::Float16_t;

std::vector<float> InferenceEngine::RunInference(const QString &modelPath, 
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

    auto pad = [](int64_t d) { return (d % 32 == 0) ? d : ((d / 32) + 1) * 32; };
    int64_t pX = pad(X);
    int64_t pY = pad(Y);
    int64_t pZ = pad(Z);

    std::vector<int64_t> inputShape = {1, C, pX, pY, pZ};
    size_t paddedSize = C * pX * pY * pZ;

    // ---- Convert to float 16 for ONNX ----
    std::vector<Ort::Float16_t> inputVector(paddedSize);
    std::fill(inputVector.begin(), inputVector.end(), Ort::Float16_t{0.0f});

    for (int c = 0; c < C; ++c) {
        for (int x = 0; x < X; ++x) {
            for (int y = 0; y < Y; ++y) {
                for (int z = 0; z < Z; ++z) {
                    size_t index = c * (pX * pY * pZ) + x * (pY * pZ) + y * pZ + z;
                    inputVector[index] = OrtFloat16(nv.data(c, x, y, z));
                }
            }
        }
    }

    // ---- Set environment (trying by DirectML, CPU if impossible) ----
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "Inference");
    Ort::SessionOptions sessionOptions;

    auto providers = Ort::GetAvailableProviders();
    if (std::find(providers.begin(), providers.end(), "DmlExecutionProvider") != providers.end()) {
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0));
        std::cout << "DirectML enabled." << std::endl;
    }

    Ort::Session session(env, modelPath.toStdWString().c_str(), sessionOptions);

    
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

        for (int c = 0; c < C; ++c) {
            for (int x = 0; x < X; ++x) {
                for (int y = 0; y < Y; ++y) {
                    for (int z = 0; z < Z; ++z) {
                        size_t index = c * (pX * pY * pZ) + x * (pY * pZ) + y * pZ + z;

                        float val = result[index];
                        outputVol.data(c, x, y, z) = std::isnan(val) ? 0.0f : val;
                    }
                }
            }
        }
        NiftiVolume::saveNifti(destinationPath, outputVol);

        return result;
    } catch (const Ort::Exception &e) {
        std::cerr << "Error during inference : " << e.what() << std::endl;
        return {};
    }

}