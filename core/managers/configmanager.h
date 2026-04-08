#pragma once

#include <QSettings>
#include <QMap>
#include <QVariant>
#include <functional>
#include <vector>

#include <utils/env_path.h>

#ifndef CORE_MANAGERS_CONFIGMANAGER_H
#define CORE_MANAGERS_CONFIGMANAGER_H

class ConfigManager {

    private:
        // Private constructor to prevent instantiation
        ConfigManager();
        QSettings settings;
        QMap<QString, std::vector<std::function<void(QVariant)>>> callbacks;

    public:

        /*
        @brief Get the singleton instance of ConfigManager
        @return ConfigManager& The singleton instance
        */
        static ConfigManager &instance();


        void set(const QString &key, const QVariant &value);
        QVariant get(const QString &key, const QVariant &defaultValue) const;
        void onChanged(const QString &key, std::function<void(QVariant)> cb);
        void save();
};

#endif