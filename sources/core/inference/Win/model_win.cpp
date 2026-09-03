#include "../model.h"
#define ENABLE_NPU_ADAPTER_ENUMERATION
#include "dml_ep_handler.h"
#include <QDebug>
#include <dml_provider_factory.h>

class ModelPrivate 
{
  public:
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memInfo{nullptr};

    /**
     * @brief Constructor that initializes the `Ort::MemoryInfo` for CPU memory allocation. This is
     * used as a default memory info for any CPU-bound tensors, ensuring that the ONNX Runtime
     * session can allocate and manage memory correctly on the CPU. The memory info is created with
     * the `OrtArenaAllocator` and `OrtMemTypeDefault` flags, which are suitable for general-purpose
     * CPU inference workloads.
     */
    ModelPrivate() { memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault); }

    /**
     * @brief Initializes the ONNX Runtime session for Windows, attempting to use OpenVINO or
     * DirectML for hardware acceleration. The function first creates an `Ort::Env` and then tries
     * to attach the OpenVINO Execution Provider (EP). If OpenVINO is not available, it falls back
     * to DirectML, checking for compatible GPU or NPU devices. If neither EP can be attached, it
     * defaults to CPU execution. The session is created with the specified model path and
     * configured with options optimized for Windows environments. Any exceptions during session
     * creation are caught and logged, with the session pointer reset to null on failure.
     * 
     * @param modelPath The file path to the ONNX model, provided as a `QString`. This is converted
     * to a wide string to comply with Windows path standards for the ONNX Runtime API.
     */
    void init(const QString &modelPath) 
    {
        env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Model_Engine");
        DMLEpHandler::registerAvailableProviders(*env, true);

        Ort::SessionOptions options;

        if (!attemptOpenVINO(options)) 
        {
            qDebug() << "OpenVINO non disponible, repli sur DirectML...";
            attemptDML(options);
        }

        options.AddConfigEntry("session.set_denorm_as_zero", "1");

        try 
        {
            session =
                std::make_unique<Ort::Session>(*env, modelPath.toStdWString().c_str(), options);
        } 
        catch (const std::exception &e) 
        {
            qDebug() << "Erreur critique lors de la création de la session ONNX:" << e.what();
            session.reset();
        }
    }

  private:
    /**
     * @brief Attempts to attach the OpenVINO Execution Provider (EP) to the ONNX Runtime session
     * options.
     * @param options The `Ort::SessionOptions` object to which the OpenVINO EP will be appended if
     * available. 
     * @return `true` if the OpenVINO EP was successfully attached, `false` if it is not available
     * or if any exception occurs during the process. The function checks the available execution
     * providers in the ONNX Runtime environment to determine if OpenVINO is present before
     * attempting to append it to the session options. If OpenVINO is not found, it logs a message
     * and returns false, allowing the caller to attempt other providers or fall back to CPU
     * execution.
     */
    bool attemptOpenVINO(Ort::SessionOptions &options) 
    {
        try 
        {
            std::vector<Ort::ConstEpDevice> ep_devices = env->GetEpDevices();
            bool hasOV = false;
            for (const auto &dev : ep_devices) 
            {
                if (std::string(dev.EpName()) == "OpenVINOExecutionProvider") 
                {
                    hasOV = true;
                    break;
                }
            }
            if (!hasOV) 
            {
                return false;
            }

            Ort::KeyValuePairs ep_options;
            qDebug() << "Target matériel : OpenVINO EP configuré.";
            return true;
        } 
        catch (...) 
        {
            return false;
        }
    }

    /**
     * @brief Attempts to attach the DirectML Execution Provider (EP) to the ONNX Runtime session options.
     * @param options The `Ort::SessionOptions` object to which the DirectML EP will be appended if
     * available. The function first checks for compatible NPU devices and attempts to attach
     * DirectML with a preference for minimum power consumption.
     * @return `true` if the DirectML EP was successfully attached with an NPU device, `false` if no
     * compatible NPU devices are found. If no NPU is available, it then checks for compatible GPU
     * devices and attempts to attach DirectML with a preference for high performance.
     */
    bool attemptDML(Ort::SessionOptions &options) 
    {
        const OrtDmlApi *dmlApi = nullptr;
        if (Ort::GetApi().GetExecutionProviderApi(
                "DML", ORT_API_VERSION, reinterpret_cast<const void **>(&dmlApi)) != nullptr) 
        {
            return false;
        }

        OrtDmlDeviceOptions devOptions;
        devOptions.Filter = OrtDmlDeviceFilter::Npu;
        devOptions.Preference = OrtDmlPerformancePreference::MinimumPower;

        if (dmlApi->SessionOptionsAppendExecutionProvider_DML2(
                static_cast<OrtSessionOptions *>(options), &devOptions) == nullptr) 
        {
            qDebug() << "Target matériel : NPU sélectionné via DML2.";
            return true;
        }

        devOptions.Filter = OrtDmlDeviceFilter::Gpu;
        devOptions.Preference = OrtDmlPerformancePreference::HighPerformance;
        if (dmlApi->SessionOptionsAppendExecutionProvider_DML2(
                static_cast<OrtSessionOptions *>(options), &devOptions) == nullptr) 
        {
            qDebug() << "Target matériel : GPU sélectionné via DML2.";
            return true;
        }

        qDebug() << "DML initialisé mais aucun accélérateur compatible trouvé. Repli CPU.";
        return false;
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
Ort::MemoryInfo& Model::getMemoryInfo() const 
{
    return d->memInfo;
}

int Model::getInputCount() const {
    if (!isValid()) {
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
 * @brief Retrieves the names of the input nodes from an ONNX model file. This function initializes an ONNX Runtime session with the specified model and queries the number of input nodes. It then iterates through each input node, retrieves its name,
 * and stores it in a vector of QStrings. If any exceptions occur during this process, they are caught and logged as warnings.
 * @param modelPath The file path to the ONNX model, provided as a `QString`. The function will attempt to initialize the ONNX Runtime session with this model and extract the input node names.
 * @return A vector of QStrings containing the names of the input nodes. If an error occurs, the vector may be empty.
 */
std::vector<QString> getModelInputNames(const QString &modelPath) {
    std::vector<QString> inputNames;
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ModelAnalysis");
        Ort::SessionOptions sessionOptions;

        Ort::Session session(env, modelPath.toStdWString().c_str(), sessionOptions);

        size_t numInputNodes = session.GetInputCount();
        Ort::AllocatorWithDefaultOptions allocator;

        for (size_t i = 0; i < numInputNodes; ++i) {
            auto inputNameAllocated = session.GetInputNameAllocated(i, allocator);
            inputNames.push_back(QString::fromUtf8(inputNameAllocated.get()));
        }
    } catch (const std::exception &e) {
        qWarning() << "Erreur lors de la lecture du modèle ONNX :" << e.what();
    }

    return inputNames;
}
