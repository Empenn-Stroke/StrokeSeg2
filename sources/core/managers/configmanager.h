// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <QSettings>
#include <QMap>
#include <QVariant>
#include <functional>
#include <vector>

#include <utils/env_path.h>

/**
 * @brief A class for managing application configuration settings.
 *
 * This class is a singleton instance for managing configuration settings
 * using QSettings. It allows setting and getting configuration values, registering
 * callbacks for changes to specific keys, and saving the configuration.
 */
class ConfigManager
{

    private:
        // Private constructor to prevent instantiation
        ConfigManager();
        QSettings settings;
        QMap<QString, std::vector<std::function<void(QVariant)>>> callbacks;

    public:
        /**
         * @brief Get the singleton instance of ConfigManager.
         *
         * @return ConfigManager& The singleton instance of ConfigManager.
         */
        static ConfigManager &instance();

        /**
         * @brief Set a configuration value for a specified key.
         *
         * @param key The key for the configuration setting.
         * @param value The value to set for the configuration setting.
         */
        void set(const QString &key, const QVariant &value);

        /**
         * @brief Get a configuration value for a specified key.
         *
         * @param key The key for the configuration setting.
         * @param defaultValue The default value to return if the key is not found.
         * @return QVariant The value of the configuration setting, or the default value if the key is not found.
         */
        QVariant get(const QString &key, const QVariant &defaultValue) const;

        /**
         * @brief Register a callback to be called when a configuration setting changes.
         *
         * @param key The key for the configuration setting to monitor for changes.
         * @param cb The callback function to be called when the configuration setting changes.
         */
        void onChanged(const QString &key, std::function<void(QVariant)> cb);

        /**
         * @brief Save the current configuration settings.
         */
        void save();
};
