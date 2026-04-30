set(HEADERS 
    "utils/animawrapper.h" 
    "utils/env_path.h" 
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
    set(OS_HEADERS "inference/Win/dml_ep_handler.h" "inference/Win/inference_win.h")
elseif(APPLE)
    set(OS_SOURCES "inference/Mac/inference_mac.cpp")
else()
    set(OS_SOURCES "inference/Linux/inference.cpp")
endif()

add_library(${CORE})

target_sources(${CORE} PRIVATE ${HEADERS} ${OS_HEADERS} ${COMMON_SOURCES} ${OS_SOURCES})

target_compile_features(${CORE} PUBLIC cxx_std_20)

set(ML_PACKAGE_ROOT "${CMAKE_BINARY_DIR}/__nuget/Microsoft.WindowsAppSDK.ML.1.8.2141")
set(ML_DLL_PATH "${ML_PACKAGE_ROOT}/runtimes-framework/win-x64/native")

target_include_directories(${CORE} PUBLIC
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
    "${ONNX_INCLUDE_DIR}"
)

if(WIN32)
    target_include_directories(${CORE} PUBLIC "${DML_INCLUDE_DIR}")
endif()

# Platform specific backends and libraries
if(WIN32)
    target_link_libraries(${CORE} PUBLIC
        zlibstatic       # Use the fetched static library on Windows
        WindowsML::Api 
        WindowsML::OnnxRuntime
    )
elseif(APPLE)
    target_link_libraries(${CORE} PUBLIC 
        ZLIB::ZLIB       # Use the system dynamic library on Mac
        onnxruntime_lib
    )
endif()

# Standard libraries used by everyone
target_link_libraries(${CORE} PUBLIC
    niftiio
    znz
    spdlog
    Qt6::Core
    eigen
    Configuration
)

# Platform specific ML backends
if(WIN32)
    target_link_libraries(${CORE} PUBLIC
        WindowsML::Api 
        WindowsML::OnnxRuntime
    )
elseif(APPLE)
    # This links to the imported library you created in the root CMakeLists.txt
    target_link_libraries(${CORE} PUBLIC onnxruntime_lib)
endif()

if(WIN32)
    add_custom_command(TARGET ${CORE} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${ML_DLL_PATH}/onnxruntime.dll"
        "${ML_DLL_PATH}/onnxruntime_providers_shared.dll"
        "${ML_DLL_PATH}/DirectML.dll"
        "${ML_DLL_PATH}/Microsoft.Windows.AI.MachineLearning.dll"
        $<TARGET_FILE_DIR:strokeseg2-app>
    )
endif()