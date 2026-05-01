#include "inference.h"

#ifdef _WIN32
#include "Win/inference_win.h"
#elif defined(__APPLE__)
#include "Mac/inference_mac.h"
#else
#include "Linux/inference_linux.h" // Todo Axel :)
#endif

#include <QDebug>
#include <QElapsedTimer>
#include <QFile>

#include "managers/progressManager.h"

NiftiVolume Inference::run(const QString &modelPath, NiftiVolume &image,
                           const QString &destinationPath, const QString &inputName,
                           const QString &outputName) {

    if (!QFile::exists(modelPath)) {
        qDebug() << modelPath;
        qDebug() << "Model not found !";
        return {};
    }

    std::string inputNameStr = inputName.toStdString();
    std::string outputNameStr = outputName.toStdString();
    const char *inputNames[] = {inputNameStr.c_str()};
    const char *outputNames[] = {outputNameStr.c_str()};

    int X = image.data.dimension(0);
    int Y = image.data.dimension(1);
    int Z = image.data.dimension(2);
    int C = image.data.dimension(3);

    qDebug() << "Shape (X:" << X << ", Y:" << Y << ", Z:" << Z << ", C:" << C << ")";

    if (!d->m_session) {
        d->init(modelPath);
    }

    Ort::IoBinding io_binding(*d->m_session);

    // --- BUG FIX: Use Eigen::half to properly cast to float16 without truncating to 0 ---
    Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor> full_volume_f16 = image.data.cast<Eigen::half>();
    image.data = Eigen::Tensor<float, 4, Eigen::ColMajor>();

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

    Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    const int64_t patch_elements = 128 * 128 * 128 * C;
    std::vector<OrtFloat16> input_buffer(patch_elements);

    const int64_t output_elements = 128 * 128 * 128 * num_classes;
    std::vector<OrtFloat16> output_buffer(output_elements);

    std::vector<int64_t> input_shape = {1, C, 128, 128, 128};
    std::vector<int64_t> output_shape = {1, num_classes, 128, 128, 128};

    Ort::Value input_tensor = Ort::Value::CreateTensor<OrtFloat16>(
        mem_info, input_buffer.data(), input_buffer.size(), input_shape.data(), input_shape.size());
    Ort::Value output_tensor =
        Ort::Value::CreateTensor<OrtFloat16>(mem_info, output_buffer.data(), output_buffer.size(),
                                             output_shape.data(), output_shape.size());

    io_binding.BindOutput(outputNameStr.c_str(), output_tensor);

    qDebug() << "Running inference on patches...";

    int patch_cpt = 0;
    int total_patches = steps[0].size() * steps[1].size() * steps[2].size();

    QElapsedTimer patch_timer;
    patch_timer.start();

    for (int x : steps[0]) {
        for (int y : steps[1]) {
            for (int z : steps[2]) {
                patch_cpt++;

                QString status =
                    QString("Processing patch %1/%2").arg(patch_cpt).arg(total_patches);

                ProgressManager::instance().report(41, 43, (patch_cpt * 100 / total_patches),
                                                   &status);

                qDebug() << "Processing patch (" << patch_cpt << "/" << total_patches
                         << ") at (X:" << x << ", Y:" << y << ", Z:" << z << ")";

                Eigen::array<int, 4> offset = {x, y, z, 0};
                Eigen::array<int, 4> extent = {128, 128, 128, C};

                // --- BUG FIX: Slice using Eigen::half and map safely ---
                Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor> patch =
                    full_volume_f16.slice(offset, extent);
                Eigen::TensorMap<Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor>> input_map(
                    reinterpret_cast<Eigen::half *>(input_buffer.data()), 128, 128, 128, C);

                input_map = patch;

                io_binding.BindInput(inputNameStr.c_str(), input_tensor);

                d->m_session->Run(Ort::RunOptions{nullptr}, io_binding);

                // --- BUG FIX: Safely map output buffer from ONNX back to Eigen::half ---
                Eigen::TensorMap<Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor>> output_map_res(
                    reinterpret_cast<Eigen::half *>(output_buffer.data()), 128, 128, 128,
                    num_classes);

                Eigen::Tensor<Eigen::half, 4, Eigen::ColMajor> out_patch = output_map_res;

                output_accum.slice(Eigen::array<int, 4>{x, y, z, 0},
                                   Eigen::array<int, 4>{128, 128, 128, num_classes}) +=
                    out_patch.cast<float>() * g_out;

                norm_map.slice(Eigen::array<int, 4>{x, y, z, 0},
                               Eigen::array<int, 4>{128, 128, 128, 1}) += g_norm;

                qDebug() << "Patch processed in" << static_cast<float>(patch_timer.elapsed()) / 1000
                         << "s";
                patch_timer.restart();
            }
        }
    }

    qDebug() << "Normalizing...";
    output_accum /= norm_map.broadcast(Eigen::array<int, 4>{1, 1, 1, num_classes});

    // --- BUG FIX: Keep both channels (Do NOT chip the tensor). Let postprocessor handle the
    // softmax ---
    NiftiVolume outputVol = image;
    outputVol.data = output_accum;

    qDebug() << "[FINAL CHECK] Dimensions to save:" << outputVol.data.dimension(0) << "x"
             << outputVol.data.dimension(1) << "x" << outputVol.data.dimension(2) << "x"
             << outputVol.data.dimension(3);

    NiftiVolume::saveNifti(destinationPath, outputVol);

    return outputVol;
}

inline std::vector<std::vector<int>> Inference::sliceVolume(std::array<int, 3> image_size,
                                                            std::array<int, 3> patch_size,
                                                            float step_size) {
    std::vector<std::vector<int>> steps;
    for (int i = 0; i < 3; ++i) {
        std::vector<int> steps_here;
        int img_dim = image_size[i];
        int pat_dim = patch_size[i];
        if (img_dim <= pat_dim) {
            steps_here.push_back(0);
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

inline Eigen::Tensor<float, 3, Eigen::ColMajor>
Inference::compute_gaussian(const std::array<int, 3> &patch_size, float sigma_scale,
                            float value_scaling_factor) {
    const int nX = patch_size[0];
    const int nY = patch_size[1];
    const int nZ = patch_size[2];

    auto get_gaussian_1d_tensor = [](int size, float sigma) {
        Eigen::Tensor<float, 1, Eigen::ColMajor> kernel(size);
        float center = (size - 1) / 2.0f;
        for (int i = 0; i < size; ++i) {
            float x = static_cast<float>(i) - center;
            kernel(i) = std::exp(-(x * x) / (2.0f * sigma * sigma));
        }
        return kernel;
    };

    auto kX = get_gaussian_1d_tensor(nX, nX * sigma_scale);
    auto kY = get_gaussian_1d_tensor(nY, nY * sigma_scale);
    auto kZ = get_gaussian_1d_tensor(nZ, nZ * sigma_scale);

    Eigen::array<int, 3> reshapeX = {nX, 1, 1};
    Eigen::array<int, 3> bcastX = {1, nY, nZ};
    Eigen::array<int, 3> reshapeY = {1, nY, 1};
    Eigen::array<int, 3> bcastY = {nX, 1, nZ};
    Eigen::array<int, 3> reshapeZ = {1, 1, nZ};
    Eigen::array<int, 3> bcastZ = {nX, nY, 1};

    Eigen::Tensor<float, 3, Eigen::ColMajor> gaussian_map = kX.reshape(reshapeX).broadcast(bcastX) *
                                                            kY.reshape(reshapeY).broadcast(bcastY) *
                                                            kZ.reshape(reshapeZ).broadcast(bcastZ);

    Eigen::Tensor<float, 0, Eigen::ColMajor> max_tensor = gaussian_map.maximum();
    float max_val = max_tensor(0);

    if (max_val > 0.0f) {
        float factor = value_scaling_factor / max_val;
        float min_val = factor * 1e-4f;
        gaussian_map = (gaussian_map * factor).cwiseMax(min_val);
    }
    return gaussian_map;
}