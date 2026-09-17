// SPDX-License-Identifier: AGPL-3.0-or-later

#pragma once

#include <WinMLEpCatalog.h>
#include <onnxruntime_cxx_api.h>

/**
 * @brief A handler class for managing DirectML (DML) execution providers in ONNX Runtime.
 *
 * This class is responsible for registering available DML providers and handling the
 * asynchronous operations related to provider management.
 */
class DMLEpHandler
{
  public:
    /**
     * @brief Register available DML execution providers.
     *
     * @param env The ONNX Runtime environment in which to register the providers.
     * @param userWantsToDownload A boolean flag indicating whether the user wants to
     *                            download the providers if they are not already available.
     */
    static void registerAvailableProviders(Ort::Env &env, bool userWantsToDownload = true);

  private:
    /**
     * @brief A structure to hold the context of the provider registration process.
     */
    struct Context {
        bool needsDownload;            /// Indicates if a download is needed.
        bool userWantsToDownload;      /// Indicates if the user wants to download the providers.
        Ort::Env *env;                  /// Pointer to the ONNX Runtime environment.
    };

    /**
     * @brief Callback function to handle progress updates during asynchronous operations.
     *
     * @param async A pointer to the WinMLAsyncBlock representing the asynchronous operation.
     * @param progress The progress of the operation as a percentage.
     */
    static void CALLBACK OnProgress(WinMLAsyncBlock *async, double progress);

    /**
     * @brief Callback function to handle the completion of asynchronous operations.
     *
     * @param async A pointer to the WinMLAsyncBlock representing the asynchronous operation.
     */
    static void CALLBACK OnComplete(WinMLAsyncBlock *async);

    /**
     * @brief Callback function to process each execution provider handle.
     *
     * @param ep A handle to the execution provider.
     * @param info A pointer to the WinMLEpInfo structure containing information about the provider.
     * @param context A pointer to the context structure.
     * @return A boolean value indicating whether to continue processing.
     */
    static BOOL CALLBACK ProcessCallback(WinMLEpHandle ep, const WinMLEpInfo *info, void *context);

    /**
     * @brief Check if the given provider name is the target provider.
     *
     * @param name The name of the provider to check.
     * @return A boolean value indicating whether the name matches the target provider.
     */
    static bool IsTargetProvider(const char *name);
};
