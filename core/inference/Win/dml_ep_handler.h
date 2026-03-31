#pragma once

#ifndef DML_EP_HANDLER_H
#define DML_EP_HANDLER_H

#include <WinMLEpCatalog.h>
#include <onnxruntime_cxx_api.h>

class DMLEpHandler {
  public:
    static void registerAvailableProviders(Ort::Env &env, bool userWantsToDownload = false);

  private:
    struct Context {
        bool needsDownload;
        bool userWantsToDownload;
        Ort::Env *env;
    };

    static BOOL CALLBACK CheckCallback(WinMLEpHandle ep, const WinMLEpInfo *info, void *context);
    static BOOL CALLBACK ProcessCallback(WinMLEpHandle ep, const WinMLEpInfo *info, void *context);
    static bool IsTargetProvider(const char *name);
};

#endif