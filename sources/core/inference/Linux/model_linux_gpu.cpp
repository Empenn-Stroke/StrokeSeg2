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

    ModelPrivate() { memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault); }


    /** 
    * @brief Initializes the ONNX Runtime session for Linux with CUDA (GPU) acceleration. The function creates an `Ort::Env` and configures session options optimized for GPU inference using NVIDIA's
    * CUDA Execution Provider (EP). It sets the number of intra-op threads to 1, which is a common baseline for GPU inference workloads. The function also adds a configuration entry to treat
    * denormalized floating-point numbers as zero, which can improve performance in certain scenarios. The ONNX model is loaded from the specified file path, and any exceptions during session creation
    * are caught and logged, with the session pointer reset to null on failure.* @brief Initializes 
    */
    void init(const QString &modelPath) 
    {
        qDebug() << "Initialisation d'ONNX Runtime sur Linux (Mode GPU/CUDA)...";

        try 
        {
            env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ONNX_Model_Engine_Linux_GPU");

            Ort::SessionOptions options;
            options.SetIntraOpNumThreads(1);
            options.AddConfigEntry("session.set_denorm_as_zero", "1");

            const OrtApi &api = Ort::GetApi();

            OrtCUDAProviderOptionsV2 *cuda_options = nullptr;
            OrtStatus *status = api.CreateCUDAProviderOptions(&cuda_options);

            if (status == nullptr) 
            {
                const char *provider_keys[] = {"device_id"};
                const char *provider_values[] = {"0"};
    
                status = api.UpdateCUDAProviderOptions(cuda_options, provider_keys, provider_values, 1);
    
                if (status == nullptr) 
                {
                    status = api.SessionOptionsAppendExecutionProvider_CUDA_V2(
                        static_cast<OrtSessionOptions *>(options), 
                        cuda_options
                    );
                }

                if (status == nullptr) 
                {
                    qDebug() << "Target matériel : CUDA EP (NVIDIA GPU) configuré avec succès.";
                } 
                else 
                {
                    const char *msg = api.GetErrorMessage(status);
                    qDebug() << "Impossible d'attacher le provider CUDA, repli sur le CPU. Raison :" << msg;
                    api.ReleaseStatus(status);
                }

                api.ReleaseCUDAProviderOptions(cuda_options);
            }
            else 
            {
                api.ReleaseStatus(status);
                qDebug() << "Impossible d'allouer les options CUDA, repli sur le CPU.";
            }

            qDebug() << "Chargement du modèle depuis :" << modelPath;

            session = std::make_unique<Ort::Session>(*env, modelPath.toUtf8().constData(), options);

            qDebug() << "Session ONNX Linux GPU/CUDA créée avec succès !";
        } 
        catch (const Ort::Exception &e) 
        {
            qDebug() << "Exception ORT critique lors de l'initialisation de la session (GPU) :" << e.what();
            session.reset();
        } 
        catch (const std::exception &e) 
        {
            qDebug() << "Exception standard critique lors de l'initialisation de la session (GPU) :" << e.what();
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
 * @param modelPath The file path to the ONNX model, provided as a `QString`. The function will attempt to initialize the ONNX Runtime session with hardware acceleration if available, and will log 
 * any errors encountered during the process.
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

Ort::MemoryInfo &Model::getMemoryInfo() const 
{
    return d->memInfo;
}

/**
 * @brief Runs a generic inference using IoBinding.
 * @param inputNames A vector of input node names as C-strings.
 * @param inputTensors A vector of input tensors.
 * @param outputNames A vector of output node names as C-strings.
 * @param outputTensors A vector to store the output tensors.
 */
void Model::run(const std::vector<const char *> &inputNames, const std::vector<Ort::Value> &inputTensors, const std::vector<const char *> &outputNames, std::vector<Ort::Value> &outputTensors) 
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
