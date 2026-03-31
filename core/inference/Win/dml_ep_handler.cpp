#include "dml_ep_handler.h"
#include <QDebug>
#include <filesystem>

const char *targetProviderNames[] = {"VitisAIExecutionProvider", "OpenVINOExecutionProvider",
                                     "QNNExecutionProvider", "NvTensorRtRtxExecutionProvider"};

QString stateToString(WinMLEpReadyState state) {
    switch (state) {
    case WinMLEpReadyState_Ready:
        return "Ready (Available)";
    case WinMLEpReadyState_NotReady:
        return "NotReady (Installation Required)";
    case WinMLEpReadyState_NotPresent:
        return "NotPresent (Hardware/Driver Missing)";
    default:
        return "Unknown";
    }
}

bool DMLEpHandler::IsTargetProvider(const char *name) {
    for (auto target : targetProviderNames) {
        if (strcmp(name, target) == 0)
            return true;
    }
    return false;
}

void DMLEpHandler::registerAvailableProviders(Ort::Env &env, bool userWantsToDownload) {
    WinMLEpCatalogHandle catalog = nullptr;
    if (FAILED(WinMLEpCatalogCreate(&catalog)))
        return;
    qDebug() << "--- Starting WinML EP Catalog Enumeration ---";

    if (FAILED(WinMLEpCatalogCreate(&catalog))) {
        qDebug() << "Error: Failed to create WinML EP Catalog.";
        return;
    }

    Context ctx = {false, userWantsToDownload, &env};

    WinMLEpCatalogEnumProviders(catalog, ProcessCallback, &ctx);

    qDebug() << "--- End of WinML EP Enumeration ---";

    //WinMLEpCatalogEnumProviders(catalog, ProcessCallback, &ctx);

    WinMLEpCatalogRelease(catalog);
}

//BOOL CALLBACK DMLEpHandler::CheckCallback(WinMLEpHandle ep, const WinMLEpInfo *info,
//                                          void *context) {
//    if (!info || !info->name)
//        return TRUE;
//    auto *ctx = static_cast<Context *>(context);
//    if (IsTargetProvider(info->name) && info->readyState == WinMLEpReadyState_NotPresent) {
//        ctx->needsDownload = true;
//    }
//    return TRUE;
//}

BOOL CALLBACK DMLEpHandler::ProcessCallback(WinMLEpHandle ep, const WinMLEpInfo *info,
                                            void *context) {
    if (!info || !info->name)
        return TRUE;

    auto *ctx = static_cast<Context *>(context);

    WinMLEpReadyState state;
    WinMLEpGetReadyState(ep, &state);

    qDebug().noquote() << QString("[%1] State: %2")
                              .arg(QString(info->name).leftJustified(32, ' '))
                              .arg(stateToString(state));

    if (IsTargetProvider(info->name)) {
        if (ctx->userWantsToDownload && state == WinMLEpReadyState_NotPresent) {
            qDebug() << "  -> Attempting WinMLEpEnsureReady for:" << info->name;
            WinMLEpEnsureReady(ep);
            WinMLEpGetReadyState(ep, &state);
        }

        if (state == WinMLEpReadyState_Ready) {
            size_t pathSize = 0;
            WinMLEpGetLibraryPathSize(ep, &pathSize);
            std::string libPath(pathSize, '\0');
            WinMLEpGetLibraryPath(ep, pathSize, libPath.data(), nullptr);

            try {
                ctx->env->RegisterExecutionProviderLibrary(
                    info->name, std::filesystem::path(libPath).wstring());
                qDebug() << "  [SUCCESS] Registered:" << info->name << "at"
                         << QString::fromStdWString(std::filesystem::path(libPath).wstring());
            } catch (const std::exception &e) {
                qDebug() << "  [FAILED] Registration error for" << info->name << ":" << e.what();
            }
        }
    }

    return TRUE;
}