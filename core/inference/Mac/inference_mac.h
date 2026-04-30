#pragma once

#include <onnxruntime_cxx_api.h>
#include <memory>
#include <vector>
#include <QString>

class InferencePrivate {
public:
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    
    // We keep buffers here if you need them for IO bindings later
    std::vector<Ort::Float16_t> m_inputBuffer;
    std::vector<Ort::Float16_t> m_outputBuffer;

    // Only the essential init function
    void init(const QString &modelPath);
};