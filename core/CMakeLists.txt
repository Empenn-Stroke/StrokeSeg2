add_library(${CORE}
  # this dummy file is only here as placeholder and can be removed
  dummy.cpp
  "utils/animawrapper.h"            "utils/animawrapper.cpp" 
  "utils/path.h" 
  "utils/str.h" 
  "preprocessing/preprocessor.h"    "preprocessing/preprocessor.cpp" 
  "utils/NiftiVolume.h"             "utils/NiftiVolume.cpp" 
  "inference/inferenceengine.h"     "inference/inferenceengine.cpp"
)

target_compile_definitions(${CORE} PRIVATE WINRT_LEAN_AND_MEAN)
target_compile_options(${CORE} PRIVATE /await)
set_target_properties(${CORE} PROPERTIES
    VS_GLOBAL_EnableCppWinRT true
)

target_compile_features(${CORE} PUBLIC cxx_std_20)

set(ML_PACKAGE_ROOT "${CMAKE_BINARY_DIR}/__nuget/Microsoft.WindowsAppSDK.ML.1.8.2109")

target_include_directories(${CORE} PRIVATE
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
    "${ML_PACKAGE_ROOT}/include"
    "${ML_PACKAGE_ROOT}/build/native/include"
)

target_link_libraries(${CORE} PUBLIC
    zlibstatic
    niftiio
    znz
    spdlog
    Qt6::Core
    eigen
    windowsapp
    winml
    Configuration
)

link_libraries(${CORE} PUBLIC
    Configuration
)

add_dependencies(${CORE} zlib)
