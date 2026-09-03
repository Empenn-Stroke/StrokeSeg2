// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <WinMLEpCatalog.h>
#include <onnxruntime_cxx_api.h>

class DMLEpHandler {
  public:
    static void registerAvailableProviders(Ort::Env &env, bool userWantsToDownload = true);

  private:
    struct Context {
        bool needsDownload;
        bool userWantsToDownload;
        Ort::Env *env;
    };

    static void CALLBACK OnProgress(WinMLAsyncBlock *async, double progress);
    static void CALLBACK OnComplete(WinMLAsyncBlock *async);
    static BOOL CALLBACK ProcessCallback(WinMLEpHandle ep, const WinMLEpInfo *info, void *context);
    static bool IsTargetProvider(const char *name);
};
