#include "mainWindow.h"
#include "warningWindow.h"
#include <QApplication>
#include <QResource>
#include <QFile>
#include <Windows.h>
#include <QCommandLineParser>
#include <workers/pipelineWorker.h>

void ensureConsole() {
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();

    fprintf(stdout, "%s\n", localMsg.constData());
    fflush(stdout);

    QString time = QDateTime::currentDateTime().toString("hh:mm:ss");

    OutputDebugStringA(("[" + time + "] " + msg + "\n").toStdString().c_str());
}

int main(int argc, char *argv[]) {
    qInstallMessageHandler(myMessageOutput);

    bool hasArgs = argc > 1;

	QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("Empenn - INRIA");
    QCoreApplication::setApplicationName("StrokeSeg2");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("StrokeSeg2 - Brain MRI Analysis Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption guiOption("gui", "Launch the application in GUI mode with prefilled options");
    QCommandLineOption inputOption({"input", "i"}, "Path to the input MRI file", "input");
    QCommandLineOption importModelOption({"import-model", "m"}, "Import an ONNX model or list models if no path given.", "path");
    QCommandLineOption modelOption("model", "Model name or path.", "model");
    QCommandLineOption onlyPreprocOption("only-preproc", "Run only brain extraction.");
    QCommandLineOption viewerOption({"viewer ", "V"}, "Viewer Name (ITK-SNAP, FSLeyes, medInria)");
    QCommandLineOption suffixOption({"suffix", "s"}, "Output file suffix");
    QCommandLineOption savePreprocOption("save-preproc", "Save all preprocessing steps");
    QCommandLineOption keepMNIOption("keep-mni", "Save output images in MNI space");
    QCommandLineOption thresholdOption({"threshold", "t"}, "Segmentation threshold", "threshold");
    QCommandLineOption pmapOption("pmap", "Save probability map");
    QCommandLineOption verboseOption("verbose", "set logging level to DEBUG instead of INFO");
    QCommandLineOption outputDirOption({"output-dir", "o"}, "Output directory for processed files",
                                       "outputDir");

    parser.addOptions({guiOption, inputOption, importModelOption, modelOption, onlyPreprocOption,
                     viewerOption, suffixOption, savePreprocOption, keepMNIOption, thresholdOption,
                     pmapOption, verboseOption, outputDirOption});

    parser.process(app);

    PipelineParams opts;
    if (parser.isSet(inputOption)) {
        opts.t1Path = parser.value(inputOption);
    }

    if (parser.isSet(modelOption)) {
        opts.modelPath = parser.value(modelOption);
    }

    if (parser.isSet(suffixOption)) {
        opts.suffix = parser.value(suffixOption);
    }

    if (parser.isSet(thresholdOption)) {
        opts.threshold = parser.value(thresholdOption).toFloat();
    }
    
    if (parser.isSet(outputDirOption)) {
        opts.outputDir = parser.value(outputDirOption);
    }
    opts.savePreproc = parser.isSet(savePreprocOption);
    opts.savePMap = parser.isSet(keepMNIOption);
    opts.skipBrainExtract = parser.isSet(onlyPreprocOption);
    opts.gui = parser.isSet(guiOption);

    if (parser.isSet(verboseOption)) {
        qDebug() << "Verbose mode enabled. Setting logging level to DEBUG.";
    }

    bool useGui = opts.gui || !hasArgs;

    if (!useGui) {
        // --- CLI mode ---
        ensureConsole();

        if (parser.isSet(importModelOption)) {
            QString modelPath = parser.value(importModelOption);
            if (modelPath.isEmpty()) {
                qDebug() << "Available models: ...";
            } else {
                qDebug() << "Importing model from:" << modelPath;
            }
            return 0;
        }

        if (parser.isSet(inputOption)) {
            qDebug() << "Processing:" << parser.value(inputOption);

            PipelineWorker(opts).process();

            return 0;
        }
    } else {
        // --- GUI mode ---

        qDebug() << "Launching in GUI mode";

        Q_INIT_RESOURCE(resources);

        QFile styleFile(":/gui/resources/style.qss");

        if (styleFile.open(QFile::ReadOnly)) {
            QString styleSheet = QTextStream(&styleFile).readAll();
            app.setStyleSheet(styleSheet);
        }

        MainWindow* w = new MainWindow(opts);

        w->show();
    }

    return app.exec();
}