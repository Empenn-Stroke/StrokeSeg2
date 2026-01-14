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

    // TEST INFERENCE ENGINE
    QString modelPath = QStringLiteral("C:/ProgramData/StrokeSeg/Models/Monomodal.onnx");

    QString imagePath = QStringLiteral("C:/Users/dvail/source/repos/MiniATLAS_2/BIDS/sub-r001s003/anat/sub-r001s003_T1w.nii.gz");
    //QString imagePath = QStringLiteral("C:/Users/dvail/source/repos/MiniATLAS_2/BIDS/derivatives/sub-r001s001/anat/sub-r001s001_T1w_MNI.nii.gz");
    //QString imagePath = QStringLiteral("C:/Users/dvail/source/repos/MiniATLAS_2/BIDS/derivatives/sub-r001s005/anat/sub-r001s005_T1w_BET.nii.gz");
    
    InferenceEngine engine;
    qDebug() << "Loading for image :" << imagePath;

    auto output = engine.RunInference(modelPath, imagePath, "input", "output");

    if (output.empty()) {
        qDebug() << "Failure : no data out";
    } else {
        qDebug() << "Success ! Output size =" << output.size();
    }
    // FIN TEST

    MainWindow w;
    w.show();



    return app.exec();
}
