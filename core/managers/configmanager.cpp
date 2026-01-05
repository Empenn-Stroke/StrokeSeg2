#include "configmanager.h"


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

void onChanged(const QString &key, std::function<void(QVariant)> cb) {
    callbacks[key].push_back(cb);
}

void save() {
    settings.sync();
}

