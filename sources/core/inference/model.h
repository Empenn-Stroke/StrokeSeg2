#pragma once

#include <QString>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <vector>

class ModelPrivate;

/**
 * @brief A class representing an ONNX model and its associated ONNX Runtime session.
 *
 * This class provides functionality to load, validate, and run inference using an ONNX model.
 * It ensures unique ownership of ONNX Runtime resources and provides methods for accessing model metadata.
 */
class Model
{
  public:
    /**
     * @brief Default constructor for the Model class.
     */
    Model();

    /**
     * @brief Destructor for the Model class.
     */
    ~Model();

    // Non-copyable but movable to ensure unique ownership of ONNX Runtime resources.
    Model(const Model &) = delete;
    Model &operator=(const Model &) = delete;
    Model(Model &&) noexcept;
    Model &operator=(Model &&) noexcept;

    /**
     * @brief Loads an ONNX model from the specified file path and initializes the ONNX Runtime
     * session.
     *
     * @param modelPath The full path to the ONNX model file.
     * @return A boolean value indicating whether the model was loaded and the session was
     *         initialized successfully.
     */
    bool load(const QString &modelPath);

    /**
     * @brief Checks if the ONNX session is initialized successfully.
     *
     * @return A boolean value indicating whether the model is valid and the session is ready for
     *         inference.
     */
    bool isValid() const;

    /**
     * @brief Runs a generic inference using IoBinding.
     *
     * @param inputNames A vector of input node names as C-strings.
     * @param inputTensors A vector of input tensors.
     * @param outputNames A vector of output node names as C-strings.
     * @param outputTensors A vector to store the output tensors.
     */
    void run(const std::vector<const char *> &inputNames,
             const std::vector<Ort::Value> &inputTensors,
             const std::vector<const char *> &outputNames, 
             std::vector<Ort::Value> &outputTensors);

    /**
     * @brief Provides access to a pre-initialized `Ort::MemoryInfo` object for CPU memory
     * allocation.
     *
     * @return A reference to the Ort::MemoryInfo object for CPU memory allocation.
     */
    Ort::MemoryInfo& getMemoryInfo() const;

    /**
     * @brief Provides the number of inputs expected by the ONNX model.
     *
     * @return The number of input nodes in the model. Returns -1 if the model is not valid or not loaded.
     */
    int getInputCount() const;

    /**
     * @brief Retrieves the names of the input nodes of the ONNX model.
     *
     * @param modelPath The full path to the ONNX model file.
     * @return A vector of input node names as QStrings. Returns an empty vector if the model is not valid or not loaded.
     */
    static std::vector<QString> getModelInputNames(const QString &modelPath);

  private:
    std::unique_ptr<ModelPrivate> d;
};