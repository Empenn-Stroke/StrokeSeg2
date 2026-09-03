// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QString>
#include "configmanager.h"

struct AppConfig {
    ConfigManager& config = ConfigManager::instance();
};


AppConfig read_application_configuration(QString ini_path);
void write_application_configuration(AppConfig config, QString ini_path);
