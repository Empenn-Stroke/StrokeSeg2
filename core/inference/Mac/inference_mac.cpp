#include "../inference.h"
#include "inference_mac.h" 
#include <QString>
#include <QDebug>

// Include the CoreML provider factory for Apple Silicon acceleration
#include <coreml_provider_factory.h> 

// 1. Initialize the unique_ptr to prevent the segfault!
Inference::Inference() : d(std::make_unique<InferencePrivate>()) {}

// 2. Destructor
Inference::~Inference() = default;

// 3. The Mac-specific Initialization
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
        // Optional: Use COREML_FLAG_ENABLE_ON_SUBGRAPH to allow partial NPU execution 
        // if some ONNX nodes aren't supported by CoreML.

        const OrtApi& api = Ort::GetApi();
        OrtStatus* status = OrtSessionOptionsAppendExecutionProvider_CoreML(static_cast<OrtSessionOptions*>(options), coreml_flags);

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