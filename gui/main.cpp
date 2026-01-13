#include "mainWindow.h"
#include "warningWindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

    QFile styleFile("../../../gui/style.qss");

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
