#include "configmanager.h"

ConfigManager &ConfigManager::instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() : settings(Paths::configPath(), QSettings::IniFormat) {
    QDir().mkpath(QFileInfo(Paths::configPath()).absolutePath());
}



void ConfigManager::set(const QString &key, const QVariant &value) {
    settings.setValue(key, value);
    if (callbacks.contains(key)) {
        for (auto &cb : callbacks[key])
            cb(value);
    }
}

QVariant ConfigManager::get(const QString &key, const QVariant &defaultValue) const {
    return settings.value(key, defaultValue);
}

void ConfigManager::onChanged(const QString &key, std::function<void(QVariant)> cb) {
    callbacks[key].push_back(cb);
}

void ConfigManager::save() {
    settings.sync();
}

