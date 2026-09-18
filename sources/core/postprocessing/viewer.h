#pragma once

#include <QString>
#include <QStringList>

#include <managers/configmanager.h>

#ifndef CORE_POSTPROCESSING_VIEWER_H
#define CORE_POSTPROCESSING_VIEWER_H

/**
 * @class Viewer
 * @brief This class handles viewers: updating paths, checking availability, and running them.
 */
class Viewer 
{
  public:
    /**
     * @brief Initialize the viewer class.
     *
     * - Setting up config, logger, and viewers (list of viewers supported by the application).
     * - Check if the default viewer is available; if not, calling for an update.
     *
     * @param config A pointer to the ConfigManager object.
     */
    Viewer(ConfigManager *config);

    /**
     * @brief Check if a viewer given is supported by the application and available on the path.
     * If not, raise an error based on the type of error. Only used in CLI mode, GUI users can simply select from a list of available viewers.
     * If the viewer is available, set it as the default one.
     *
     * @param viewer The name of the viewer as a QString.
     */
    void CheckViewers(const QString &viewer);

    /**
     * @brief Open the base image and the generated segmentation in the default viewer.
     * If an error occurs, call for an update of the viewers paths.
     *
     * @param imgPath The base image path (input) as a QString.
     * @param segPath The binary mask path (output) as a QString.
     */
    void Run(const QString &imgPath, const QString &segPath);

  private:
    ConfigManager* m_config;
    QStringList m_viewers;

    QString itkSnapExe() const; // Helper to get the correct shortcut depending on the OS

    /**
     * @brief Update all the paths for the viewers supported by the application.
     * Set a viewer available on the path as the default one.
     */
    void UpdatePath();
};

#endif
