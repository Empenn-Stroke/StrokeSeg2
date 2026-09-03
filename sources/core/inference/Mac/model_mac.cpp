// SPDX-License-Identifier: AGPL-3.0-or-later

#include "../model.h"
#include <QDebug>
#include <QString>
#include <stdexcept>
#include <coreml_provider_factory.h>

class ModelPrivate 
{
  public:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memInfo{nullptr};

    /**
     * @brief Constructor that initializes the `Ort::MemoryInfo` for CPU memory allocation. This is used as a default memory info for any CPU-bound tensors, ensuring that the ONNX Runtime
     * session can allocate and manage memory correctly on the CPU. The memory info is created with the `OrtArenaAllocator` and `OrtMemTypeDefault` flags, which are suitable for general-purpose CPU inference workloads.
     */
    ModelPrivate() { memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault); }

    /**
     * @brief Initializes the ONNX Runtime session for macOS, specifically targeting CoreML (Apple Silicon / GPU / Neural Engine) execution. The function creates an `Ort::Env` and configures session options optimized for macOS inference. It sets the number of intra-op threads to 1, which is a common baseline for inference workloads. The function also adds a configuration entry to treat denormalized floating-point numbers as zero, which can improve performance in certain scenarios. The ONNX model is loaded from the specified file path, and any exceptions during session creation are caught and logged, with the session pointer reset to null on failure.
     * @param modelPath Absolute path to the ONNX model file, provided as a `QString`. The function converts this path to a UTF-8 encoded C-string to comply with macOS POSIX path standards.
     */
    void init(const QString &modelPath) 
    {
        qDebug() << "Initialisation d'ONNX Runtime sur macOS...";

        try 
        {
            env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Model_Engine_macOS");

            Ort::SessionOptions options;
            options.SetIntraOpNumThreads(1);
            options.AddConfigEntry("session.set_denorm_as_zero", "1");

            // --- CoreML ---
            uint32_t coremlFlags = 0;
            coremlFlags |= COREML_FLAG_ENABLE_ON_SUBGRAPH;

            const OrtApi &api = Ort::GetApi();
            OrtStatus *status = OrtSessionOptionsAppendExecutionProvider_CoreML(
                static_cast<OrtSessionOptions *>(options), coremlFlags);

            if (status == nullptr) 
            {
                qDebug() << "Fournisseur d'exécution CoreML configuré avec succès.";
            } 
            else 
            {
                const char *msg = api.GetErrorMessage(status);
                qDebug() << "Impossible d'ajouter l'EP CoreML, repli sur le CPU. Raison :" << msg;
                api.ReleaseStatus(status);
            }

            qDebug() << "Chargement du modèle depuis :" << modelPath;

            session = std::make_unique<Ort::Session>(*env, modelPath.toUtf8().constData(), options);

            qDebug() << "Session ONNX macOS créée avec succès !";
        } 
        catch (const Ort::Exception &e) 
        {
            qDebug() << "Exception ORT critique lors de l'initialisation de la session macOS :" << e.what();
            session.reset();
        } 
        catch (const std::exception &e) 
        {
            qDebug() << "Exception standard critique lors de l'initialisation de la session macOS :" << e.what();
            session.reset();
        }
    }
};

Model::Model() : d(std::make_unique<ModelPrivate>()) {}
Model::~Model() = default;
Model::Model(Model &&) noexcept = default;
Model &Model::operator=(Model &&) noexcept = default;

/**
 * @brief Loads an ONNX model from the specified file path and initializes the ONNX Runtime session.
 */
bool Model::load(const QString &modelPath)
{
    bool success = false;

    if (!modelPath.isEmpty())
	{
        d->init(modelPath);
        success = isValid();
    }

    return success;
}

/**
 * @brief Checks if the ONNX session is initialized successfully.
 */
bool Model::isValid() const
{
    return d && d->session != nullptr;
}

/**
 * @brief Provides access to the CPU `Ort::MemoryInfo` object.
 */
Ort::MemoryInfo &Model::getMemoryInfo() const
{
    return d->memInfo;
}

int Model::getInputCount() const
{
    if (!isValid())
	{
        return -1;
    }
    return static_cast<int>(d->session->GetInputCount());
}

/**
 * @brief Executes a generic inference using IoBinding.
 */
void Model::run(const std::vector<const char *> &inputNames, 
                const std::vector<Ort::Value> &inputTensors, 
                const std::vector<const char *> &outputNames, 
                std::vector<Ort::Value> &outputTensors)
{
    if (!isValid())
	{
        throw std::runtime_error("Impossible d'exécuter l'inférence : le modèle n'est pas chargé.");
    }

    Ort::IoBinding ioBinding(*d->session);

    for (size_t i = 0; i < inputNames.size(); ++i)
	{
        ioBinding.BindInput(inputNames[i], inputTensors[i]);
    }
    for (size_t i = 0; i < outputNames.size(); ++i)
	{
        ioBinding.BindOutput(outputNames[i], outputTensors[i]);
    }

    d->session->Run(Ort::RunOptions{nullptr}, ioBinding);
}

/**
 * @brief Static method defined in model.h : Retrieves the names of the input nodes.
 */
std::vector<QString> Model::getModelInputNames(const QString &modelPath) 
{
    std::vector<QString> inputNames;
    try 
    {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ModelAnalysis");
        Ort::SessionOptions sessionOptions;

        Ort::Session session(env, modelPath.toUtf8().constData(), sessionOptions);

        size_t numInputNodes = session.GetInputCount();
        Ort::AllocatorWithDefaultOptions allocator;

        for (size_t i = 0; i < numInputNodes; ++i) 
        {
            auto inputNameAllocated = session.GetInputNameAllocated(i, allocator);
            inputNames.push_back(QString::fromUtf8(inputNameAllocated.get()));
        }
    } 
    catch (const std::exception &e) 
    {
        qWarning() << "Erreur lors de la lecture du modèle ONNX :" << e.what();
    }

    return inputNames;
}
