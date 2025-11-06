#include "mainWindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

    QFile styleFile("../../../gui/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QTextStream(&styleFile).readAll();
        app.setStyleSheet(styleSheet);
    }

    MainWindow w;
    w.show();

    return app.exec();
}