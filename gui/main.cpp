#include "mainWindow.h"
#include "warningWindow.h"
#include <QApplication>
#include <QFile>
#include <inference/inferenceengine.h>

int main(int argc, char *argv[]) {

	QApplication app(argc, argv);

    QFile styleFile("../../../gui/style.qss");

    if (styleFile.open(QFile::ReadOnly)) {
        QString styleSheet = QTextStream(&styleFile).readAll();
        app.setStyleSheet(styleSheet);
    }

    QCoreApplication::setOrganizationName("Empenn - INRIA");
    QCoreApplication::setApplicationName("StrokeSeg2");


    InferenceEngine engine; // To ensure static initialization
    auto output = engine.RunInference(
        "C:/ProgramData/StrokeSeg/Models/Bimodal.onnx",
        "C:/Users/dvail/Desktop/Laïcan/Code/Pindus/MiniATLAS_2/BIDS/sub-r001s003/anat/sub-r001s003_T1w.nii.gz"
    );

    qDebug() << "Output size =" << output.size();

    MainWindow w;
    w.show();



    return app.exec();
}