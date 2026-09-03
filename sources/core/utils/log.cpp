// SPDX-License-Identifier: AGPL-3.0-or-later

#include "log.h"
#include <QDebug>

void printAction(const QString &actionName) 
{
    qInfo().noquote() << QString("Starting %1...").arg(actionName);
}