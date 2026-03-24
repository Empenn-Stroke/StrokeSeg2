#pragma once

#include <QDir>
#include <preprocessing/preprocessor.h>
#include <preprocessing/resampling.h>
#include <inference/inferenceengine.h>
#include <postprocessing/postprocessor.h>
#include <utils/niftiVolume.h>
#include <utils/path.h>

struct PipelineParams {
    QString t1Path;
    QString modelPath;
    QString outputDir;
    QString suffix;
    float threshold;
    bool savePMap;
    bool savePreproc;
    bool skipBrainExtract;
};

class PipelineWorker : public QObject {
    Q_OBJECT
  public:
    PipelineWorker(PipelineParams &p) : m_p(p) {}

  public slots:
    void process();

  signals:
    void statusChanged(QString message);
    void finished(bool success, QString message, QString finalPath);
    void progressUpdated(int percent);
  private:
    PipelineParams m_p;
};