#include "mainWindow.h"
#include "warningWindow.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <Windows.h>
#include <managers/logManager.h>
#include <workers/pipelineWorker.h>

void ensureConsole() {
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) {
        FILE *out = nullptr;
        FILE *err = nullptr;
        freopen_s(&out, "CONOUT$", "w", stdout);
        freopen_s(&err, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio();
    }
}

int main(int argc, char *argv[]) {
    QCoreApplication::setOrganizationName("Empenn - INRIA");
    QCoreApplication::setApplicationName("StrokeSeg2");

    qInstallMessageHandler(LogManager::messageHandler);

    QApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    LogManager::cleanupOldLogs(); // Remove logs older than 30 days

    QCommandLineParser parser;
    parser.setApplicationDescription("StrokeSeg2 - Brain MRI Analysis Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption guiOption("gui", "Launch in GUI mode");
    QCommandLineOption inputOption({"input", "i"}, "Input MRI file", "input");
    QCommandLineOption modelOption("model", "Model name or path", "model");
    QCommandLineOption onlyPreprocOption("only-preproc", "Run only brain extraction");
    QCommandLineOption suffixOption({"suffix", "s"}, "Output suffix", "suffix");
    QCommandLineOption savePreprocOption("save-preproc", "Save all preprocessing steps");
    QCommandLineOption keepMNIOption("keep-mni", "Save in MNI space");
    QCommandLineOption thresholdOption({"threshold", "t"}, "Segmentation threshold", "threshold");
    QCommandLineOption pmapOption("pmap", "Save probability map");
    QCommandLineOption verboseOption("verbose", "Set logging level to DEBUG");
    QCommandLineOption outputDirOption({"output-dir", "o"}, "Output directory", "outputDir");
    QCommandLineOption skipPreprocOption("skip-preproc", "Skip preprocessing");
    QCommandLineOption listModelsOption("list-models", "List models");

    parser.addOptions({guiOption, inputOption, modelOption, onlyPreprocOption, suffixOption,
                       savePreprocOption, keepMNIOption, thresholdOption, pmapOption, verboseOption,
                       outputDirOption, skipPreprocOption, listModelsOption});

    parser.process(app);

    PipelineParams opts;

    if (parser.isSet(listModelsOption)) {
        ensureConsole();
        QDir modelsDir = Paths::modelDir();
        QStringList entries = modelsDir.entryList({"*.onnx"}, QDir::Files);
        qDebug() << "--- Available Models ---";
        for (const QString &model : entries) {
            qDebug() << "  ->" << model.section('.', 0, 0);
        }
        return 0;
    }

    if (parser.isSet(inputOption))
        opts.t1Path = parser.value(inputOption);

    if (parser.isSet(modelOption)) {
        QString modelInput = parser.value(modelOption);
        if (!modelInput.contains('/') && !modelInput.contains('\\') &&
            !modelInput.endsWith(".onnx")) {
            QString fullPath = Paths::modelDir().absoluteFilePath(modelInput + ".onnx");
            opts.modelPath = QFile::exists(fullPath) ? fullPath : modelInput;
        } else {
            opts.modelPath = modelInput;
        }
    }

    if (parser.isSet(suffixOption))
        opts.suffix = parser.value(suffixOption);
    if (parser.isSet(thresholdOption))
        opts.threshold = parser.value(thresholdOption).toFloat();
    if (parser.isSet(outputDirOption))
        opts.outputDir = parser.value(outputDirOption);

    opts.savePreProcessing = parser.isSet(savePreprocOption);
    opts.savePMap = parser.isSet(pmapOption);
    opts.mni = parser.isSet(keepMNIOption);
    opts.skipPreProcessing = parser.isSet(skipPreprocOption);
    opts.betOnly = parser.isSet(onlyPreprocOption);
    opts.gui = parser.isSet(guiOption);

    bool useGui = opts.gui || (argc == 1);

    if (!useGui) {
        ensureConsole();
        if (opts.t1Path.isEmpty()) {
            qCritical() << "Error: No input file specified.";
            return 1;
        }
        PipelineWorker worker(opts);
        return worker.process();
    } else {
        Q_INIT_RESOURCE(resources);
        QFile styleFile(":/gui/resources/style.qss");
        if (styleFile.open(QFile::ReadOnly)) {
            app.setStyleSheet(QTextStream(&styleFile).readAll());
        }
        MainWindow *w = new MainWindow(opts);
        w->show();
        return app.exec();
    }
}