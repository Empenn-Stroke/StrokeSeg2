#include "../inference.h"
#include "inference_win.h"

#include "dml_ep_handler.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QFile>
#include <cmath>
#include <array>

Inference::Inference() : d(std::make_unique<InferencePrivate>()) {}
Inference::~Inference() = default;

bool Inference::loadModel(const QString &modelPath) {
    try {
        d->init(modelPath);
        return true;
    } catch (const std::exception &e) {
        qDebug() << "Load error:" << e.what();
        return false;
    }
}


/* @brief Initialize the ONNX Runtime session with the specified model path. This method is
 *        called internally by loadModel() after loading the model.
 * @param modelPath The full path to the ONNX model file to load (including the .onnx
 *        extension).
 * @throws std::runtime_error if the model cannot be loaded or if there is an error initializing
 *         the ONNX Runtime session.
 */
inline void InferencePrivate::init(const QString &modelPath) {
    m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "InferenceWin");

    // Download/Register missing plugins (like OpenVINO)
    DMLEpHandler::registerAvailableProviders(*m_env, true);

    Ort::SessionOptions options;

    // Priority 1: Try OpenVINO
    bool deviceFound = attemptOpenVINO(options);

    // Priority 2: Fallback to DirectML
    if (!deviceFound) {
        qDebug() << "Falling back to DirectML...";
        deviceFound = attemptDML(options);
    }

    options.AddConfigEntry("session.set_denorm_as_zero", "1");

    try {
        m_session =
            std::make_unique<Ort::Session>(*m_env, modelPath.toStdWString().c_str(), options);
    } catch (const std::exception &e) {
        qDebug() << "Session Error:" << e.what();
        m_session.reset();
    }
}

/* @brief Attempt to configure the ONNX Runtime session to use DirectML (DML) as the execution
 * provider.
 *
 * @param options The SessionOptions object to configure with DML if available.
 *
 * @return true if DML was successfully configured as the execution provider, false otherwise.
 */
/* @brief Attempt to configure the ONNX Runtime session to use OpenVINO.
 * @return true if OpenVINO was successfully configured, false otherwise.
 */
inline bool InferencePrivate::attemptOpenVINO(Ort::SessionOptions &options) {
    try {
        // 1. Enumerate EP devices available in your environment
        std::vector<Ort::ConstEpDevice> ep_devices = m_env->GetEpDevices();
        std::vector<Ort::ConstEpDevice> selected_ep_devices;

        // 2. Collect OpenVINO devices (You can optionally filter for NPU specifically here)
        for (const auto &d : ep_devices) {
            if (std::string(d.EpName()) == "OpenVINOExecutionProvider") {
                // If you ONLY want OpenVINO on the NPU, uncomment the next line:
                // if (d.HardwareDevice().Type() == OrtHardwareDeviceType_NPU)
                selected_ep_devices.push_back(d);
            }
        }

        if (selected_ep_devices.empty()) {
            qDebug() << "OpenVINO EP is registered, but no compatible target devices were found.";
            return false;
        }

        // 3. Configure provider-specific options and append
        Ort::KeyValuePairs ep_options;
        // Note: You can add specific OpenVINO configuration keys here if needed
        // ep_options.Add("device_type", "NPU");

        options.AppendExecutionProvider_V2(*m_env, {selected_ep_devices.front()}, ep_options);

        qDebug() << "OpenVINO Execution Provider targeted successfully via V2 API.";
        return true;

    } catch (const std::exception &e) {
        qDebug() << "Failed to configure OpenVINO:" << e.what();
        return false;
    }
}

/**
 * @brief Attempts to configure the DirectML (DML) Execution Provider for ONNX Runtime.
 *
 * This method checks for the availability of the DirectML API and attempts to attach a 
 * hardware accelerator to the provided session options. It uses the DML2 API and 
 * follows a strict hardware priority sequence:
 *   1. **NPU (Neural Processing Unit):** Targeted first, optimized for minimum power consumption.
 *   2. **GPU (Graphics Processing Unit):** Targeted second, optimized for high performance.
 *
 * If the DML API is unavailable, or if both NPU and GPU initialization fail, the function 
 * logs the failure and falls back to CPU execution.
 *
 * @param options A reference to the ONNX Runtime session options (`Ort::SessionOptions`) 
 *                that will be modified if a DML device is successfully appended.
 *
 * @return `true` if either the NPU or GPU was successfully configured as the execution provider.
 * @return `false` if the DML API is missing, or if no compatible NPU/GPU could be targeted.
 */
inline bool InferencePrivate::attemptDML(Ort::SessionOptions &options) {
    const OrtDmlApi *dmlApi = nullptr;
    if (Ort::GetApi().GetExecutionProviderApi(
            "DML", ORT_API_VERSION, reinterpret_cast<const void **>(&dmlApi)) != nullptr) {
        return false;
    }

    OrtDmlDeviceOptions devOptions;
    // Priorité 1 : NPU
    devOptions.Filter = OrtDmlDeviceFilter::Npu;
    devOptions.Preference = OrtDmlPerformancePreference::MinimumPower;
    if (dmlApi->SessionOptionsAppendExecutionProvider_DML2(
            static_cast<OrtSessionOptions *>(options), &devOptions) == nullptr) {
        qDebug() << "NPU targeted successfully via DML2 API.";
        return true;
    }

    // Priorité 2 : GPU
    devOptions.Filter = OrtDmlDeviceFilter::Gpu;
    devOptions.Preference = OrtDmlPerformancePreference::HighPerformance;
    if (dmlApi->SessionOptionsAppendExecutionProvider_DML2(
            static_cast<OrtSessionOptions *>(options), &devOptions) == nullptr) {
        qDebug() << "Discrete GPU targeted successfully via DML2 API.";
        return true;
    }

    qDebug() << "DML found but no NPU/GPU compatible, falling back to CPU.";
    return false;
}