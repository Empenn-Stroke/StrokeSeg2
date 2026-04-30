#pragma once

#include <onnxruntime_cxx_api.h>
#include <memory>
#include <QString>

class InferencePrivate {
  public:
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;

    // We don't need the attemptDML or attemptOpenVINO helpers here!
    void init(const QString &modelPath);
};