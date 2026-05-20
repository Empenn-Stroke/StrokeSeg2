set(HEADERS 
    "utils/animawrapper.h"
    "utils/str.h" 
    "utils/NiftiVolume.h" 
    "utils/log.h"
    "utils/dicomConverter.h"
    "preprocessing/preprocessor.h" 
    "preprocessing/resampling.h" 
    "preprocessing/brainextraction.h"
    "postprocessing/postprocessor.h" 
    "managers/configmanager.h"
    "managers/appconfig.h"
    "managers/progressManager.h"
    "managers/logManager.h"
    "inference/inference.h"
    "workers/pipelineWorker.h")

set(COMMON_SOURCES 
    "utils/animawrapper.cpp" 
    "utils/NiftiVolume.cpp" 
    "utils/log.cpp"
    "utils/dicomConverter.cpp"
    "preprocessing/preprocessor.cpp" 
    "preprocessing/resampling.cpp" 
    "preprocessing/brainextraction.cpp"
    "postprocessing/postprocessor.cpp" 
    "managers/configmanager.cpp" 
    "managers/progressManager.cpp"
    "managers/logManager.cpp"
    "inference/inference.cpp"
    "workers/pipelineWorker.cpp")


if(WIN32)
    set(OS_SOURCES "inference/Win/inference_win.cpp" "inference/Win/dml_ep_handler.cpp")
    set(OS_HEADERS "inference/Win/dml_ep_handler.h" "inference/Win/inference_win.h" "utils/env_path.h")
elseif(APPLE)
    set(OS_SOURCES "inference/Mac/inference_mac.cpp")
else()
    set(OS_SOURCES "inference/Linux/inference.cpp")
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
elseif(APPLE)
    target_link_libraries(${CORE} PUBLIC
        onnxruntime::onnxruntime
    )
endif()