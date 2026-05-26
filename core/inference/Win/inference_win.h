#include <onnxruntime_cxx_api.h>

using OrtFloat16 = Ort::Float16_t;

class InferencePrivate {
  public:
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::vector<Ort::Float16_t> m_inputBuffer;
    std::vector<Ort::Float16_t> m_outputBuffer;

    /* @brief Initialize the ONNX Runtime session with the specified model path. This method is
     *        called internally by loadModel() after loading the model.
     * @param modelPath The full path to the ONNX model file to load (including the .onnx
     *        extension).
     * @throws std::runtime_error if the model cannot be loaded or if there is an error initializing
     *         the ONNX Runtime session.
     */
    void init(const QString &modelPath);

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
    bool attemptOpenVINO(Ort::SessionOptions &options);

    bool attemptDML(Ort::SessionOptions &options);
};