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

    // --- Check if the model file exists and initialize the session ---
    if (QFile::exists(fullPath)) {
        this->initSession(fullPath);
    }
}

void InferenceEngine::initSession(const QString &modelPath) {
    // --- Initialize ONNX Runtime environment and session options ---
    env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "Inference");
    Ort::SessionOptions sessionOptions;

    // --- Check if DirectML execution provider is available and use it if possible ---
    auto providers = Ort::GetAvailableProviders();
    if (std::find(providers.begin(), providers.end(), "DmlExecutionProvider") != providers.end()) {
        Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0));
        sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
    }

    for (const auto &provider : providers) {
        qDebug() << "Available ONNX Runtime provider:" << QString::fromStdString(provider);
    }

    session =
        std::make_unique<Ort::Session>(*env, modelPath.toStdWString().c_str(), sessionOptions);
}

std::vector<std::vector<int>> InferenceEngine::sliceVolume(std::array<int, 3> image_size,
                                                           std::array<int, 3> patch_size,
                                                           float step_size) {
    std::vector<std::vector<int>> steps;

    // --- For each dimension, calculate the starting indices for patches ---
    for (int i = 0; i < 3; ++i) {
        std::vector<int> steps_here;
        int img_dim = image_size[i];
        int pat_dim = patch_size[i];

        // If the patch size is larger than the image dimension, we just take one patch starting at 0 
        // (after preprocessing, image should be at least the dimension of the patch, but we keep
        // this check for safety)
        if (img_dim <= pat_dim) {
            steps_here.push_back(0);

        // Otherwise, we calculate the steps based on the step size and ensure we cover the
        // entire dimension
        } else {
            float target_step = pat_dim * step_size;
            int num_steps = std::ceil((img_dim - pat_dim) / target_step) + 1;

            float actual_step = static_cast<float>(img_dim - pat_dim) / (num_steps - 1);

            for (int j = 0; j < num_steps; ++j) {
                int start_idx = std::round(actual_step * j);

                if (start_idx + pat_dim > img_dim) {
                    start_idx = img_dim - pat_dim;
                }

                if (steps_here.empty() || start_idx != steps_here.back()) {
                    steps_here.push_back(start_idx);
                }
            }
        }
        steps.push_back(steps_here);
    }
    return steps;
}

Eigen::Tensor<float, 3, Eigen::ColMajor>
InferenceEngine::compute_gaussian(const std::array<int, 3> &patch_size, float sigma_scale,
                                   float value_scaling_factor) {
    int nX = patch_size[0];
    int nY = patch_size[1];
    int nZ = patch_size[2];

    Eigen::Tensor<float, 3, Eigen::ColMajor> gaussian_map(nX, nY, nZ);

    // --- Generate 1D Gaussian kernels for each dimension ---
    auto get_gaussian_1d = [](int size, float sigma) {
        std::vector<float> kernel(size);
        float center = (size - 1) / 2.0f;
        for (int i = 0; i < size; ++i) {
            float x = static_cast<float>(i) - center;
            kernel[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        }
        return kernel;
    };

    std::vector<float> kX = get_gaussian_1d(nX, nX * sigma_scale);
    std::vector<float> kY = get_gaussian_1d(nY, nY * sigma_scale);
    std::vector<float> kZ = get_gaussian_1d(nZ, nZ * sigma_scale);

    // --- Compute the 3D Gaussian map ---
    for (int z = 0; z < nZ; ++z) {
        for (int y = 0; y < nY; ++y) {
            float yz_prod = kY[y] * kZ[z];
            for (int x = 0; x < nX; ++x) {
                gaussian_map(x, y, z) = kX[x] * yz_prod;
            }
        }
    }

    // --- Normalize the Gaussian map to have a maximum value of value_scaling_factor ---
    float max_val = 0.0f;
    for (int i = 0; i < gaussian_map.size(); ++i)
        max_val = std::max(max_val, gaussian_map.data()[i]);

    if (max_val > 0) {
        float factor = value_scaling_factor / max_val;
        float min_val = factor * 1e-4f;
        for (int i = 0; i < gaussian_map.size(); ++i) {
            gaussian_map.data()[i] *= factor;
            if (gaussian_map.data()[i] <= 0)
                gaussian_map.data()[i] = min_val;
        }
    }

    return gaussian_map;
}





NiftiVolume InferenceEngine::run(const QString &modelPath, const QString &imagePath,
                                 const QString &destinationPath, const QString &inputName,
                                 const QString &outputName) {

    if (!QFile::exists(modelPath) || !QFile::exists(imagePath)) {
        qDebug() << modelPath;
        qDebug() << "Image or model not found !";
        return {};
    }

    std::string inputNameStr = inputName.toStdString();
    std::string outputNameStr = outputName.toStdString();
    const char *inputNames[] = {inputNameStr.c_str()};
    const char *outputNames[] = {outputNameStr.c_str()};

    qDebug() << "Loading image from :" << imagePath;
    NiftiVolume nv = NiftiVolume::loadNifti(imagePath);

    int X = nv.data.dimension(0);
    int Y = nv.data.dimension(1);
    int Z = nv.data.dimension(2);
    int C = nv.data.dimension(3);

    qDebug() << "Shape (X:" << X << ", Y:" << Y << ", Z:" << Z << ", C:" << C << ")";

    // --- Initialize ONNX Runtime session and tensors for future computation ---
    initSession(modelPath);

    std::array<int, 3> patch_size = {128, 128, 128};
    auto steps = sliceVolume({X, Y, Z}, patch_size, 0.5f);
    auto gaussian = compute_gaussian(patch_size);

    int num_classes = 2;
    Eigen::Tensor<float, 4, Eigen::ColMajor> output_accum(X, Y, Z, num_classes);
    Eigen::Tensor<float, 4, Eigen::ColMajor> norm_map(X, Y, Z, 1);
    output_accum.setZero();
    norm_map.setZero();

    Eigen::array<int, 4> bcast_out = {1, 1, 1, num_classes};
    Eigen::array<int, 4> bcast_norm = {1, 1, 1, 1};
    Eigen::Tensor<float, 4, Eigen::ColMajor> g_out =
        gaussian.reshape(Eigen::array<int, 4>{128, 128, 128, 1}).broadcast(bcast_out);
    Eigen::Tensor<float, 4, Eigen::ColMajor> g_norm =
        gaussian.reshape(Eigen::array<int, 4>{128, 128, 128, 1}).broadcast(bcast_norm);

    qDebug() << "Running inference on patches...";

    int patch_cpt = 0;
    for (int x : steps[0]) {
        for (int y : steps[1]) {
            for (int z : steps[2]) {
                patch_cpt++;
                qDebug() << "Processing patch (" << patch_cpt << "/8) at (X:" << x << ", Y:" << y << ", Z:" << z << ")";

                // --- Extract patch and prepare input tensor ---
                Eigen::array<int, 4> offset = {x, y, z, 0};
                Eigen::array<int, 4> extent = {128, 128, 128, C};
                Eigen::Tensor<float, 4, Eigen::ColMajor> patch_col = nv.data.slice(offset, extent);
                
                // --- Convert to float16 and prepare for ONNX Runtime ---
                Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor> patch_onnx_ready =
                    patch_col.cast<OrtFloat16>();

                // --- Create input tensor for ONNX Runtime ---
                std::vector<OrtFloat16> patch_vec(patch_onnx_ready.size());
                std::copy(patch_onnx_ready.data(),
                          patch_onnx_ready.data() + patch_onnx_ready.size(), patch_vec.begin());
                std::vector<int64_t> patch_shape = {1, C, 128, 128, 128};
                Ort::MemoryInfo mem_info =
                    Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
                Ort::Value input_tensor = Ort::Value::CreateTensor<OrtFloat16>(
                    mem_info, patch_vec.data(), patch_vec.size(), patch_shape.data(),
                    patch_shape.size());

                // --- Run inference ---
                auto output_tensors = session->Run(Ort::RunOptions{nullptr}, inputNames,
                                                   &input_tensor, 1, outputNames, 1);

                // --- Process output tensor ---
                OrtFloat16 *raw_data = output_tensors[0].GetTensorMutableData<OrtFloat16>();

                Eigen::TensorMap<Eigen::Tensor<OrtFloat16, 4, Eigen::ColMajor>> output_map(
                    raw_data, 128, 128, 128, num_classes);

                Eigen::Tensor<float, 4, Eigen::ColMajor> pred_col = output_map.cast<float>();

                // --- Accumulate results with Gaussian weighting ---
                output_accum.slice(Eigen::array<int, 4>{x, y, z, 0},
                                   Eigen::array<int, 4>{128, 128, 128, num_classes}) +=
                    pred_col * g_out;

                // --- Accumulate normalization map ---
                norm_map.slice(Eigen::array<int, 4>{x, y, z, 0},
                               Eigen::array<int, 4>{128, 128, 128, 1}) += g_norm;
            }
        }
    }

    qDebug() << "Normalizing...";
    output_accum /= norm_map.broadcast(Eigen::array<int, 4>{1, 1, 1, num_classes});

    qDebug() << "Extracting class 1 and reshaping to 3D...";

    auto class_slice_4d =
        output_accum.slice(Eigen::array<int, 4>{0, 0, 0, 1}, Eigen::array<int, 4>{X, Y, Z, 1});

    Eigen::array<int, 3> shape_3d = {X, Y, Z};
    Eigen::Tensor<float, 3, Eigen::ColMajor> final_tensor_3d = class_slice_4d.reshape(shape_3d);

    NiftiVolume outputVol = nv;
    outputVol.data = final_tensor_3d.reshape(Eigen::array<int, 4>{X, Y, Z, 1});

    qDebug() << "[FINAL CHECK] Dimensions to save:" << outputVol.data.dimension(0) << "x"
             << outputVol.data.dimension(1) << "x" << outputVol.data.dimension(2) << "x"
             << outputVol.data.dimension(3);

    NiftiVolume::saveNifti(destinationPath, outputVol);

    return outputVol;
}
