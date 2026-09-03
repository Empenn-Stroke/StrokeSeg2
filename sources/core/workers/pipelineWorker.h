// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QDir>
#include <preprocessing/preprocessor.h>
#include <preprocessing/resampling.h>
#include <postprocessing/postprocessor.h>
#include <utils/niftiVolume.h>
#include <utils/env_path.h>
#include <utils/pipelineParams.h>



class PipelineWorker : public QObject
{
    Q_OBJECT
  public:
    PipelineWorker(PipelineParams &p);

  public slots:
    int process();

  signals:
    void statusChanged(QString message);
    void finished(bool success, QString message, QString finalPath);
    void progressUpdated(int percent);

  private:
    PipelineParams m_p;
    AnimaWrapper *m_wrapper;
};
