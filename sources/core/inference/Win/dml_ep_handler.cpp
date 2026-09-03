// SPDX-License-Identifier: AGPL-3.0-or-later

#include "dml_ep_handler.h"
#include <WinMLAsync.h>
#include <thread>
#include <QDebug>
#include <filesystem>

//const char *targetProviderNames[] = {"VitisAIExecutionProvider", "OpenVINOExecutionProvider", "NvTensorRtRtxExecutionProvider"};

std::array<const char *, 3> targetProviderNames = {"VitisAIExecutionProvider", "OpenVINOExecutionProvider", "NvTensorRtRtxExecutionProvider"};

QString stateToString(WinMLEpReadyState state) 
{
    switch (state) 
    {
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

bool DMLEpHandler::IsTargetProvider(const char *name) 
{
    bool result = false;

    for (auto it = targetProviderNames.begin(); !result && it != targetProviderNames.end(); ++it)
    {
        result = result || (strcmp(name, *it) == 0);
    }

    return result;
}

void DMLEpHandler::registerAvailableProviders(Ort::Env &env, bool userWantsToDownload) {
    WinMLEpCatalogHandle catalog = nullptr;

    qDebug() << "--- Starting WinML EP Catalog Enumeration ---";

    if (FAILED(WinMLEpCatalogCreate(&catalog))) 
    {
        qDebug() << "Error: Failed to create WinML EP Catalog.";
        return;
    }

    Context ctx = {false, userWantsToDownload, &env};

    WinMLEpCatalogEnumProviders(catalog, ProcessCallback, &ctx);

    qDebug() << "--- End of WinML EP Enumeration ---";

    WinMLEpCatalogRelease(catalog);
}

void CALLBACK DMLEpHandler::OnProgress(WinMLAsyncBlock *async, double progress) {
    // progress is out of 100, convert to 0-1 range
    double normalizedProgress = progress / 100.0;

    // Display the progress to the user
    qDebug() << std::format("Progress: {:.0f}%", normalizedProgress * 100);
}

void CALLBACK DMLEpHandler::OnComplete(WinMLAsyncBlock *async) {
    HRESULT hr = WinMLAsyncGetStatus(async, FALSE);
    if (SUCCEEDED(hr)) 
    {
        qDebug() << "Download complete!\n";
    } 
    else 
    {
        qDebug() << std::format("Download failed: 0x{:08X}", static_cast<uint32_t>(hr));
    }
}

void CALLBACK OnEnsureReadyComplete(WinMLAsyncBlock *async) 
{
    // 1. On récupère le statut (juste pour le log)
    HRESULT hr = WinMLAsyncGetStatus(async, FALSE);
    qDebug() << "[WinML-Async] Operation terminated in background. Code : 0x"
             << QString::number(static_cast<uint32_t>(hr), 16);

    WinMLAsyncClose(async);

    delete async;
    qDebug() << "[WinML-Async] Memory released, no crash possible.";
}

BOOL CALLBACK DMLEpHandler::ProcessCallback(WinMLEpHandle ep, const WinMLEpInfo *info,
                                            void *context) 
{
    if (!info || !info->name) 
    {
        return TRUE;
    }

    auto *ctx = static_cast<Context *>(context);
    WinMLEpReadyState state;
    WinMLEpGetReadyState(ep, &state);

    qDebug().noquote() << QString("[%1] State: %2")
                              .arg(QString(info->name).leftJustified(32, ' '))
                              .arg(stateToString(state));

    if (IsTargetProvider(info->name) && ctx->userWantsToDownload) 
    {
        if (state == WinMLEpReadyState_NotPresent || state == WinMLEpReadyState_NotReady) 
        {

            qDebug() << "  -> Attempting WinMLEpEnsureReady for:" << info->name;

            WinMLAsyncBlock *async = new WinMLAsyncBlock{};

            async->callback = OnEnsureReadyComplete;
            async->progress = nullptr;
            
            HRESULT hr = WinMLEpEnsureReadyAsync(ep, async);

            if (FAILED(hr)) 
            {
                qDebug() << "  [ERREUR] Download failed : 0x" << QString::number(hr, 16);
                delete async;
            } 
            else 
            {
                qDebug() << "  [INFO] Request sent, resuming computation...";
            }
        }

        if (state == WinMLEpReadyState_Ready) 
        {
            size_t pathSize = 0;
            WinMLEpGetLibraryPathSize(ep, &pathSize);
            std::string libPath(pathSize, '\0');
            WinMLEpGetLibraryPath(ep, pathSize, libPath.data(), nullptr);

            try 
            {
                ctx->env->RegisterExecutionProviderLibrary(
                    info->name, std::filesystem::path(libPath).wstring());
                qDebug() << "  [SUCCESS] Registered:" << info->name << "at"
                         << QString::fromStdWString(std::filesystem::path(libPath).wstring());
            } 
            catch (const std::exception &e) {
                qDebug() << "  [FAILED] Registration error for" << info->name << ":" << e.what();
            }
        }
    }

    return TRUE;
}
