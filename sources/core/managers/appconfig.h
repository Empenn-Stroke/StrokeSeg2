// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QString>
#include "configmanager.h"

/**
 * @brief A structure representing application configuration settings.
 *
 * This structure encapsulates the configuration settings for the application,
 * utilizing a reference to the singleton instance of the ConfigManager.
 */
struct AppConfig {
    ConfigManager& config = ConfigManager::instance();
};

/**
 * @brief Reads application configuration from an INI file.
 *
 * This function reads the application configuration settings from the specified INI file
 * and returns them in an AppConfig structure.
 *
 * @param ini_path The path to the INI file containing the configuration settings.
 * @return An AppConfig structure containing the read configuration settings.
 */
AppConfig read_application_configuration(QString ini_path);

/**
 * @brief Writes application configuration to an INI file.
 *
 * This function writes the application configuration settings from the provided AppConfig
 * structure to the specified INI file.
 *
 * @param config The AppConfig structure containing the configuration settings to write.
 * @param ini_path The path to the INI file where the configuration settings should be written.
 */
void write_application_configuration(AppConfig config, QString ini_path);
