#include "mainWindow.h"
#include "warningWindow.h"
#include <QApplication>
#include <QResource>
#include <QFile>

int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

    Q_INIT_RESOURCE(resources);

    QFile styleFile(":/gui/resources/style.qss");

    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QTextStream(&styleFile).readAll();
        app.setStyleSheet(styleSheet);
    }

    QCoreApplication::setOrganizationName("Empenn - INRIA");
    QCoreApplication::setApplicationName("StrokeSeg2");

    MainWindow w;
    w.show();



    return app.exec();
}
