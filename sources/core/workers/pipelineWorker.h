// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QDir>
#include <preprocessing/preprocessor.h>
#include <preprocessing/resampling.h>
#include <postprocessing/postprocessor.h>
#include <utils/niftiVolume.h>
#include <utils/env_path.h>
#include <utils/pipelineParams.h>
#include <QObject>

/**
 * @brief A worker class responsible for executing the StrokeSeg2 pipeline.
 *
 * This class manages the workflow of the StrokeSeg2 pipeline, including preprocessing,
 * inference, and postprocessing steps. It communicates with other components
 * through signals and slots to report status, completion, and progress.
 */
class PipelineWorker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructor for PipelineWorker.
     *
     * @param p Reference to the PipelineParams object containing the configuration
     *          and parameters for the pipeline execution.
     */
    PipelineWorker(PipelineParams &p);

  public slots:
    /**
     * @brief Executes the StrokeSeg2 pipeline.
     *
     * This method orchestrates the entire pipeline process, including loading input
     * data, applying preprocessing, running the model inference, and performing
     * postprocessing. It emits signals to update the UI on status, progress, and
     * completion.
     *
     * @return An integer representing the exit status of the pipeline execution.
     */
    int process();

  signals:
    /**
     * @brief Signal emitted when the status of the pipeline changes.
     *
     * @param message A QString containing the current status message.
     */
    void statusChanged(QString message);

    /**
     * @brief Signal emitted when the pipeline execution is finished.
     *
     * @param success A boolean indicating whether the pipeline completed successfully.
     * @param message A QString containing a message describing the result.
     * @param finalPath A QString containing the path to the final output file.
     */
    void finished(bool success, QString message, QString finalPath);

    /**
     * @brief Signal emitted to update the progress of the pipeline execution.
     *
     * @param percent An integer representing the current progress percentage.
     */
    void progressUpdated(int percent);

private:
    PipelineParams m_p; /**< The PipelineParams object containing configuration and parameters. */
    AnimaWrapper *m_wrapper; /**< Pointer to the AnimaWrapper object for processing. */
};
