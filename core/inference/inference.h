#ifndef CORE_INFERENCE_INFERENCE_H
#define CORE_INFERENCE_INFERENCE_H

#include <QString>
#include <memory>
#include "utils/NiftiVolume.h"

class Inference {
  public:
    Inference();
    ~Inference();

    bool loadModel(const QString &modelPath);
    NiftiVolume run(const QString &modelPath, const QString &imagePath,
                    const QString &destinationPath, const QString &inputName,
                    const QString &outputName);

  private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};

#endif