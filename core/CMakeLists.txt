add_library(${CORE}
    "utils/animawrapper.h"              "utils/animawrapper.cpp" 
    "utils/path.h" 
    "utils/str.h" 
    "utils/NiftiVolume.h"               "utils/NiftiVolume.cpp" 
    "preprocessing/preprocessor.h"      "preprocessing/preprocessor.cpp" 
    "preprocessing/resampling.h"        "preprocessing/resampling.cpp"
    "preprocessing/brainextraction.h"   "preprocessing/brainextraction.cpp"
    "inference/inferenceengine.h"       "inference/inferenceengine.cpp"
    "postprocessing/postprocessor.h"    "postprocessing/postprocessor.cpp"
    "managers/configmanager.h"          "managers/configmanager.cpp"
    "managers/appconfig.h"
)

target_compile_features(${CORE} PUBLIC cxx_std_20)

set(ML_PACKAGE_ROOT "${CMAKE_BINARY_DIR}/__nuget/Microsoft.WindowsAppSDK.ML.1.8.2109")
set(ML_LIB_PATH "${ML_PACKAGE_ROOT}/lib/native/x64")

set(ONNX_PATH "${CMAKE_BINARY_DIR}/__nuget/Microsoft.ML.OnnxRuntime.DirectML.1.17.1")
set(DML_PATH  "${CMAKE_BINARY_DIR}/__nuget/Microsoft.AI.DirectML.1.13.1")

set(ONNX_INCLUDE_DIR "${ONNX_PATH}/build/native/include")
set(DML_INCLUDE_DIR  "${DML_PATH}/include")

set(ONNX_LIB_DIR "${ONNX_PATH}/runtimes/win-x64/native")
set(DML_LIB_DIR "${DML_PATH}/bin/x64-win")

target_include_directories(${CORE} PUBLIC
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
    "${ONNX_INCLUDE_DIR}"
    "${DML_INCLUDE_DIR}"
)

target_link_libraries(${CORE} PUBLIC
    zlibstatic
    niftiio
    znz
    spdlog
    Qt6::Core
    eigen
    "${ONNX_LIB_DIR}/onnxruntime.lib"
    "${DML_LIB_DIR}/DirectML.lib"
    Configuration
)

add_custom_command(TARGET ${CORE} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${ONNX_LIB_DIR}/onnxruntime.dll"
    "${DML_LIB_DIR}/DirectML.dll"
    $<TARGET_FILE_DIR:${CORE}>
)

add_dependencies(${CORE} zlib)