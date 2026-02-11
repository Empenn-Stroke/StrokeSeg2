#include "utils/log.h"

void printAction(const QString &actionName) {
    spdlog::info("Starting {}...", actionName.toStdString());
}