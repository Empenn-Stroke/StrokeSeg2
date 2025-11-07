#include <cstdlib>      // std::getenv
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QString>
#include "str.h"


static inline QString roaming = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
static inline QString local = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
static inline QString programData =
    QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);

static inline const QString base_dir =
    QDir(QFileInfo(__FILE__).absolutePath() + "/..").canonicalPath();
static inline const QString config_file = QDir(roaming).filePath(app_name + "/config.ini");
static inline const QString anima_dir =
    QFileInfo(QDir(base_dir).filePath("../anima")).canonicalFilePath();
static inline const QString model_dir = QDir(programData).filePath(app_name + "/Models");
static inline const QString atlas_dir = QDir(programData).filePath(app_name + "/Atlas");
static inline const QString logo_inria = QDir(base_dir).filePath("assets/INRIA.png");
static inline const QString logo_institutions =
    QDir(base_dir).filePath("assets/LOGO_INSTITUTIONS.png");
static inline const QString logo = QDir(base_dir).filePath("assets/StrokeSeg.png");
static inline const QString log_dir = QDir(local).filePath(app_name + "strokeseg.log");
static inline const QString USER_GUIDE = QDir(base_dir).filePath("USER_GUIDE.md");
