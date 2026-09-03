// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mainWindow.h"
#include "warningWindow.h"
#include <managers/tempCleanupManager.h>
#include <QApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QtGlobal>
#include <QSettings>
#include <managers/logManager.h>
#include <utils/modelManifest.h>
#include <workers/pipelineWorker.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#include <fcntl.h>
#include <io.h>
#endif

void ensureConsole()
{
#ifdef Q_OS_WIN
    if (AttachConsole(ATTACH_PARENT_PROCESS) || AllocConsole()) 
    {
        FILE *out = nullptr;
        FILE *err = nullptr;
        freopen_s(&out, "CONOUT$", "w", stdout);
        freopen_s(&err, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio();
    }
#endif
}

/**
 * @brief Parses the --input option values into the params' inputPaths map.
 * Accepts "CHANNEL=path" (e.g. "T1=/data/t1.nii.gz") or a bare path, which is
 * then assumed to be the T1 (reference) channel for backward compatibility.
 */
void parseInputOptions(const QStringList &rawValues, PipelineParams &opts)
{
    for (const QString &raw : rawValues) 
    {
        int sepIndex = raw.indexOf('=');
        bool hasChannel = (sepIndex > 0);

        QString channel = hasChannel ? raw.left(sepIndex).trimmed().toUpper() : QString("T1");
        QString path = hasChannel ? raw.mid(sepIndex + 1).trimmed() : raw.trimmed();

        if (!path.isEmpty())
        {
            opts.inputPaths[channel] = path;
        }
    }
}

int main(int argc, char *argv[]) 
{
    int exitCode = 0;
    bool done = false;

    QCoreApplication::setApplicationName("StrokeSeg2");
    QCoreApplication::setOrganizationName("INRIA - Empenn");

    qInstallMessageHandler(LogManager::messageHandler);

    QApplication app(argc, argv);
    app.setApplicationVersion("1.0");

    LogManager::cleanupOldLogs();

    QSettings startupSettings;
    QStringList recentOutputDirs = startupSettings.value("recentOutputDirs").toStringList();
    TempCleanupManager::cleanupOldTempDirs(recentOutputDirs, 7);

    QCommandLineParser parser;
    parser.setApplicationDescription("StrokeSeg2 - Brain MRI Analysis Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption guiOption("gui", "Launch in GUI mode");
    QCommandLineOption inputOption({"input", "i"}, "Input MRI file (CHANNEL=path, e.g. T1=/path/t1.nii.gz). Repeatable.", "input");
    QCommandLineOption modelOption("model", "Model name or path", "model");
    QCommandLineOption onlyPreprocOption("only-preproc", "Run only brain extraction");
    QCommandLineOption suffixOption({"suffix", "s"}, "Output suffix", "suffix");
    QCommandLineOption saveInterStepsOption("save-inter-steps", "Save all preprocessing and postprocessing steps");
    QCommandLineOption keepMNIOption("keep-mni", "Save in MNI space");
    QCommandLineOption thresholdOption({"threshold", "t"}, "Segmentation threshold", "threshold", "0.5");
    QCommandLineOption pmapOption("pmap", "Save probability map");
    QCommandLineOption verboseOption("verbose", "Set logging level to DEBUG");
    QCommandLineOption outputDirOption({"output-dir", "o"}, "Output directory", "outputDir");
    QCommandLineOption skipPreprocOption("skip-preproc", "Skip preprocessing");
    QCommandLineOption skipInferenceOption("skip-inference", "Skip inference");
    QCommandLineOption skipPostprocOption("skip-postproc", "Skip postprocessing");
    QCommandLineOption noBrainExtractionOption("no-brain-extraction", "Skip brain extraction (use if input images are already skull-stripped)");
    QCommandLineOption listModelsOption("list-models", "List models");

    parser.addOptions({guiOption, inputOption, modelOption, onlyPreprocOption, suffixOption,
                       saveInterStepsOption, keepMNIOption, thresholdOption, pmapOption, verboseOption,
                       outputDirOption, skipPreprocOption, skipInferenceOption, skipPostprocOption,
                       noBrainExtractionOption, listModelsOption});

    parser.process(app);

    PipelineParams opts;

    if (parser.isSet(listModelsOption)) 
    {
        ensureConsole();
        QDir modelsDir = Paths::modelDir();
        QStringList entries = modelsDir.entryList({"*.onnx"}, QDir::Files);
        qDebug() << "--- Available Models ---";
        for (const QString &model : entries)
        {
            qDebug() << "  ->" << model.section('.', 0, 0);
        }
        exitCode = 0;
        done = true;
    }

    if (!done)
    {
        if (parser.isSet(inputOption))
        {
            parseInputOptions(parser.values(inputOption), opts);
        }

        if (parser.isSet(modelOption))
        {
            QString modelInput = parser.value(modelOption);
            if (!modelInput.contains('/') && !modelInput.contains('\\') && !modelInput.endsWith(".onnx")) 
            {
                QString fullPath = Paths::modelDir().absoluteFilePath(modelInput + ".onnx");
                opts.modelPath = QFile::exists(fullPath) ? fullPath : modelInput;
            } 
            else
            {
                opts.modelPath = modelInput;
            }
        }

        if (parser.isSet(suffixOption)) 
        {
            opts.suffix = parser.value(suffixOption);
        }
        if (parser.isSet(outputDirOption)) 
        {
            opts.outputDir = parser.value(outputDirOption);
        }

        opts.threshold = parser.value(thresholdOption).toFloat();
        opts.saveInterSteps = parser.isSet(saveInterStepsOption);
        opts.savePMap = parser.isSet(pmapOption);
        opts.mni = parser.isSet(keepMNIOption);
        opts.skipPreProcessing = parser.isSet(skipPreprocOption);
        opts.skipInference = parser.isSet(skipInferenceOption);
        opts.skipPostProcessing = parser.isSet(skipPostprocOption);
        opts.betOnly = parser.isSet(onlyPreprocOption);
        opts.brainExtraction = !parser.isSet(noBrainExtractionOption);
        opts.gui = parser.isSet(guiOption);

        if (!opts.modelPath.isEmpty()) 
        {
            opts.modalities = ModelManifest::loadModalities(opts.modelPath);

            opts.inputOrder.clear();
            for (const Modality &m : opts.modalities) 
            {
                opts.inputOrder << m.name;
            }
        }

        bool useGui = opts.gui || (argc == 1);

        if (!useGui)
        {
            ensureConsole();

            bool canProceed = true;

            if (opts.inputPaths.isEmpty())
            {
                qCritical() << "Error: No input file specified.";
                exitCode = 1;
                canProceed = false;
            }

            if (canProceed && opts.outputDir.isEmpty())
            {
                qCritical() << "Error: No output directory specified (--output-dir).";
                exitCode = 1;
                canProceed = false;
            }

            QString referencePath;

            if (canProceed)
            {
                if (opts.inputOrder.isEmpty() || !opts.inputPaths.contains(opts.inputOrder.first()))
                {
                    qCritical() << "Error: No reference input defined for this model.";
                    exitCode = 1;
                    canProceed = false;
                }
                else
                {
                    referencePath = opts.inputPaths.value(opts.inputOrder.first());
                }
            }

            if (canProceed)
            {
                QString modelName = QFileInfo(opts.modelPath).baseName();
                QString subjectName = QFileInfo(referencePath).baseName();
                opts.outputDir = opts.outputDir + "/" + subjectName + "_" + modelName;

                if (!QDir().mkpath(opts.outputDir))
                {
                    qCritical() << "Error: Failed to create output directory:" << opts.outputDir;
                    exitCode = 1;
                    canProceed = false;
                }
            }

            if (canProceed)
            {
                PipelineWorker worker(opts);

                QObject::connect(&worker, &PipelineWorker::statusChanged, [](QString msg) 
                {
                    qInfo().noquote() << "[STATUS]" << msg;
                });

                QObject::connect(&worker, &PipelineWorker::finished, [&exitCode](bool success, QString message, QString finalPath) 
                {
                    if (success) 
                    {
                        qInfo().noquote() << "[SUCCESS]" << message;
                        if (!finalPath.isEmpty()) 
                        {
                            qInfo().noquote() << "Output file:" << finalPath;
                        }
                    }
                    else 
                    {
                        qCritical().noquote() << "[FAILED]" << message;
                        exitCode = 1;
                    }
                });

                int workerResult = worker.process();
                if (workerResult != 0) 
                {
                    exitCode = 1;
                }
            }

            done = true;
        }
        else 
        {
            Q_INIT_RESOURCE(resources);

            QFile styleFile(":/gui/resources/style.qss");
            if (styleFile.open(QFile::ReadOnly))
            {
                QString styleSheet = QLatin1String(styleFile.readAll());
                app.setStyleSheet(styleSheet);
            }

            MainWindow *w = new MainWindow(opts);
            w->show();
            exitCode = app.exec();
            done = true;
        }
    }

    return exitCode;
}
