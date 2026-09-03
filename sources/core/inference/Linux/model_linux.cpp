// SPDX-License-Identifier: AGPL-3.0-or-later

#include "../model.h"
#include <QDebug>
#include <QString>
#include <stdexcept>

class ModelPrivate 
{
  public:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memInfo{nullptr};

    /**
     * @brief Constructor that initializes the `Ort::MemoryInfo` for CPU memory allocation. This is used as a default memory info for any CPU-bound tensors, ensuring that the ONNX Runtime
     * session can allocate and manage memory correctly on the CPU. The memory info is created with the `OrtArenaAllocator` and `OrtMemTypeDefault` flags, which are suitable for general-purpose
     * CPU inference workloads.
     */
    ModelPrivate() { memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault); }

    /**
     * @brief Initializes the ONNX Runtime session for Linux, specifically targeting CPU execution. The function creates an `Ort::Env` and configures session options optimized for Linux CPU 
     * inference. It sets the number of intra-op threads to 1, which is a common baseline for CPU inference workloads. The function also adds a configuration entry to treat denormalized 
     * floating-point numbers as zero, which can improve performance in certain scenarios. The ONNX model is loaded from the specified file path, and any exceptions during session creation are caught
     * and logged, with the session pointer reset to null on failure.
     * @param modelPath Absolute path to the ONNX model file, provided as a `QString`. The function converts this path to a UTF-8 encoded C-string to comply with Linux POSIX path standards.
     */
    void init(const QString &modelPath) 
    {
        qDebug() << "Initialisation d'ONNX Runtime sur Linux (Mode CPU)...";

        try 
        {
            env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Model_Engine_Linux_CPU");

            Ort::SessionOptions options;
            options.SetIntraOpNumThreads(1);
            options.AddConfigEntry("session.set_denorm_as_zero", "1");

            qDebug() << "Target matériel : CPU par défaut.";
            qDebug() << "Chargement du modèle depuis :" << modelPath;

            session = std::make_unique<Ort::Session>(*env, modelPath.toUtf8().constData(), options);

            qDebug() << "Session ONNX Linux CPU créée avec succès !";
        } 
        catch (const Ort::Exception &e) 
        {
            qDebug() << "Exception ORT critique lors de l'initialisation de la session (CPU) :" << e.what();
            session.reset();
        } 
        catch (const std::exception &e) 
        {
            qDebug() << "Exception standard critique lors de l'initialisation de la session (CPU) :" << e.what();
            session.reset();
        }
    }
};


Model::Model() : d(std::make_unique<ModelPrivate>()) {}
Model::~Model() = default;
Model::Model(Model &&) noexcept = default;
Model &Model::operator=(Model &&) noexcept = default;

/**
 * @brief Loads an ONNX model from the specified file path and initializes the ONNX Runtime
 * session.
 * @param modelPath The file path to the ONNX model, provided as a `QString`. The function will
 * attempt to initialize the ONNX Runtime session with hardware acceleration if available, and
 * will log any errors encountered during the process.
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
 * @brief Provides access to a pre-initialized `Ort::MemoryInfo` object for CPU memory
 * allocation.
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
 * @brief Runs a generic inference using IoBinding.
 * @param inputNames A vector of input node names as C-strings.
 * @param inputTensors A vector of input tensors.
 * @param outputNames A vector of output node names as C-strings.
 * @param outputTensors A vector to store the output tensors.
 */
void Model::run(const std::vector<const char *> &inputNames, const std::vector<Ort::Value> &inputTensors, const std::vector<const char *> &outputNames, std::vector<Ort::Value> &outputTensors) {
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
 * @brief Retrieves the names of the input nodes from an ONNX model file. This function initializes an ONNX Runtime session with the specified model and queries the number of input nodes. It then iterates through each input node, retrieves its name,
 * and stores it in a vector of QStrings. If any exceptions occur during this process, they are caught and logged as warnings.
 * @param modelPath The file path to the ONNX model, provided as a `QString`. The function will attempt to initialize the ONNX Runtime session with this model and extract the input node names.
 * @return A vector of QStrings containing the names of the input nodes. If an error occurs, the vector may be empty.
 */
std::vector<QString> getModelInputNames(const QString &modelPath) 
{
    std::vector<QString> inputNames;
    try 
    {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ModelAnalysis");
        Ort::SessionOptions sessionOptions;

        Ort::Session session(env, modelPath.toStdString().c_str(), sessionOptions);

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
