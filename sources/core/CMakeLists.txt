# SPDX-License-Identifier: AGPL-3.0-or-later

set(HEADERS 
    "utils/animawrapper.h"
    "utils/niftiVolume.h" 
    "utils/log.h"
    "utils/pipelineParams.h"
    "utils/dicomConverter.h"
    "utils/modelManifest.h"
    "utils/modality.h"
    "preprocessing/preprocessor.h" 
    "preprocessing/resampling.h" 
    "preprocessing/brainextraction.h"
    "preprocessing/preprocVolume.h"
    "postprocessing/postprocessor.h" 
    "managers/configmanager.h"
    "managers/appconfig.h"
    "managers/progressManager.h"
    "managers/logManager.h"
    "managers/tempCleanupManager.h"
    "inference/inference.h"
    "inference/model.h"
    "workers/pipelineWorker.h")

set(COMMON_SOURCES 
    "utils/animawrapper.cpp" 
    "utils/niftiVolume.cpp" 
    "utils/log.cpp"
    "utils/dicomConverter.cpp"
    "utils/modelManifest.cpp"
    "preprocessing/preprocessor.cpp" 
    "preprocessing/resampling.cpp" 
    "preprocessing/brainextraction.cpp"
    "postprocessing/postprocessor.cpp" 
    "managers/configmanager.cpp" 
    "managers/progressManager.cpp"
    "managers/logManager.cpp"
    "managers/tempCleanupManager.cpp"
    "inference/inference.cpp"
    "workers/pipelineWorker.cpp")


if(WIN32)
    set(OS_SOURCES "inference/Win/dml_ep_handler.cpp" "inference/Win/model_win.cpp")
    set(OS_HEADERS "inference/Win/dml_ep_handler.h" "utils/env_path.h")
elseif(APPLE)
    set(OS_SOURCES "inference/Mac/model_mac.cpp")
else()
    if(GPU)
        set(OS_SOURCES "inference/Linux/model_linux_gpu.cpp")
    else()
        set(OS_SOURCES "inference/Linux/model_linux.cpp")
    endif()
endif()

add_library(${CORE})

target_sources(${CORE} PRIVATE ${HEADERS} ${OS_HEADERS} ${COMMON_SOURCES} ${OS_SOURCES})

target_compile_features(${CORE} PUBLIC cxx_std_20)

set(ML_PACKAGE_ROOT "${CMAKE_BINARY_DIR}/../../../nuget_packages/Microsoft.WindowsAppSDK.ML.1.8.2141")
set(ML_DLL_PATH "${ML_PACKAGE_ROOT}/runtimes-framework/win-x64/native")

target_include_directories(${CORE} PUBLIC
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
    "${ONNX_INCLUDE_DIR}"
)

if(GPU)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GPU)
endif()

if(WIN32)
    target_include_directories(${CORE} PUBLIC "${DML_INCLUDE_DIR}")
endif()

target_link_libraries(${CORE} PUBLIC
    ZLIB::ZLIB
    Qt6::Core            
    Qt6::Gui             
    Qt6::Concurrent
    NIFTI::niftiio
    Eigen3::Eigen
    Configuration
)

if(WIN32)
    target_link_libraries(${CORE} PUBLIC
        WindowsML::Api 
        WindowsML::OnnxRuntime
        WindowsML::WindowsML
    )
else()
    target_link_libraries(${CORE} PUBLIC
        onnxruntime::onnxruntime
    )

    set(ONNX_LIB_DIR "${onnxruntime_DIR}/../../")

    if(NOT APPLE AND GPU)
        find_library(ORT_CUDA_PROVIDER
            NAMES onnxruntime_providers_cuda
            HINTS "${ONNX_LIB_DIR}"
        )

        message (STATUS "[StrokeSeg Core] GPU setting activated, searching for ONNX Runtime CUDA Provider in: ${ONNX_LIB_DIR}")

        if(ORT_CUDA_PROVIDER)
            target_link_libraries(${CORE} PRIVATE ${ORT_CUDA_PROVIDER})
            message(STATUS "[StrokeSeg Core] ONNX Runtime CUDA Provider found : ${ORT_CUDA_PROVIDER}")
        else()
            message(FATAL_ERROR "[StrokeSeg Core] GPU setting activated but libonnxruntime_providers_cuda.so not found !")
        endif()
    endif()
endif()
