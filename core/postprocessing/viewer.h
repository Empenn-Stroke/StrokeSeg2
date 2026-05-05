#pragma once

#include <QString>
#include <QStringList>

#include <managers/configmanager.h>

#ifndef CORE_POSTPROCESSING_VIEWER_H
#define CORE_POSTPROCESSING_VIEWER_H

/**
* @class Viewer
* @brief This class handle viewers : updating path, checking availability and running it
*/
class Viewer 
{
  public:
    /**
     * @brief Initialize the viewer class
     *
     * - Setting up config, logger and viewers (List of viewers supported by the application)
     * 
     * - Check if the default viewer is available; if not, calling for an update.
     */
    Viewer(ConfigManager *config);

    /**
    * @brief Check if a viewer given is supported by the application and available on the path. If not raise an error based on the type of error. 
      Only used in CLI mode, GUI users can simply select from a list of available viewers. If the viewer is available, set it as default one
      @param viewer(QString): name of the viewer

    */
	void CheckViewers(const QString &viewer);

    /**
    * @brief Open the base image and the generated segmentation in the default viewer. If an error occurs, call for an update of the viewers paths
    * @param imgPath(QString): Base image path (input)
    * @param segPath(QString): Binary mask path (output)
    */
	void Run(const QString &imgPath, const QString &segPath);

  private:
    ConfigManager* m_config;
    QStringList m_viewers;

    QString itkSnapExe() const; //Helper to get the correct shortcut dependin on the OS

    /**
    * @brief Update all the path for the viewers supported by the application. Set a viewer available on the path as the default one
    */
	void UpdatePath();
};

#endif