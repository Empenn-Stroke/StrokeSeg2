#include "../inference.h"
#include "inference_mac.h" 
#include <QString>
#include <QDebug>
#include <coreml_provider_factory.h> 

Inference::Inference() : d(std::make_unique<InferencePrivate>()) {}

Inference::~Inference() = default;

/**
 * @brief Initializes the ONNX Runtime session specifically for macOS environments.
 *
 * This method sets up the `Ort::Env` and configures session options optimized for macOS.
 * It primarily attempts to enable hardware acceleration by attaching the CoreML 
 * Execution Provider (EP), which leverages Apple Silicon (Apple Neural Engine / GPU). 
 * The `COREML_FLAG_ENABLE_ON_SUBGRAPH` flag is used to allow partial acceleration 
 * even if the entire model isn't CoreML-compatible.
 * 
 * If CoreML initialization fails, the function logs the error and safely falls back 
 * to CPU execution.
 *
 * @param modelPath The file path to the ONNX model, provided as a `QString`. 
 *                  This is converted to a standard UTF-8 C-string to comply with 
 *                  macOS POSIX path standards.
 *
 * @exception Ort::Exception Caught internally and logged if ONNX Runtime fails to build the session.
 * @exception std::exception Caught internally and logged for standard C++ errors.
 * 
 * @note In the event of an exception during initialization, the internal session 
 *       pointer (`m_session`) is safely reset to null.
 */
void InferencePrivate::init(const QString& modelPath) {
    qDebug() << "Initializing ONNX Runtime on macOS...";

    try {
        // Create the environment
        m_env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "InferenceMac");

        Ort::SessionOptions options;
        options.SetIntraOpNumThreads(1); // Standard baseline
        options.AddConfigEntry("session.set_denorm_as_zero", "1");

        // --- Attempt to attach CoreML (Apple Neural Engine / GPU) ---
        uint32_t coreml_flags = 0; 
        coreml_flags |= COREML_FLAG_ENABLE_ON_SUBGRAPH;

        const OrtApi& api = Ort::GetApi();
        OrtStatus* status = OrtSessionOptionsAppendExecutionProvider_CoreML(
            static_cast<OrtSessionOptions*>(options), coreml_flags);
        if (status == nullptr) {
            qDebug() << "CoreML Execution Provider targeted successfully.";
        } else {
            const char* msg = api.GetErrorMessage(status);
            qDebug() << "Could not append CoreML EP, falling back to CPU. Reason:" << msg;
            api.ReleaseStatus(status);
        }

        // --- Create the Session ---
        qDebug() << "Loading model from:" << modelPath;
        
        // On macOS, file paths are standard UTF-8 chars, unlike Windows wstring.
        m_session = std::make_unique<Ort::Session>(*m_env, modelPath.toUtf8().constData(), options);
        
        qDebug() << "macOS ONNX Session created successfully!";

    } catch (const Ort::Exception& e) {
        qDebug() << "ONNX Runtime ORT Exception during init:" << e.what();
        m_session.reset();
    } catch (const std::exception& e) {
        qDebug() << "Standard Exception during init:" << e.what();
        m_session.reset();
    }
}