add_library(${CORE}
  # this dummy file is only here as placeholder and can be removed
  dummy.cpp
 "utils/animawrapper.cpp" "utils/animawrapper.h" "utils/path.h" "utils/str.h" "preprocessing/preprocessor.h" "preprocessing/preprocessor.cpp" "utils/NiftiVolume.h" "utils/NiftiVolume.cpp" "inference/inferenceengine.h" "inference/inferenceengine.cpp")


target_compile_features(${CORE} PUBLIC cxx_std_20)
target_include_directories(${CORE} PRIVATE
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
    "${CMAKE_BINARY_DIR}/__nuget/Microsoft.WindowsAppSDK.ML.1.8.2109/include"
)

target_link_libraries(${CORE} PUBLIC
    zlibstatic
    niftiio
    znz
    spdlog
    Qt6::Core
    eigen
)

link_libraries(${CORE} PUBLIC
    Configuration
)

add_dependencies(${CORE} zlib)

if(WIN32)
    target_link_libraries(${CORE} PRIVATE
        "${CMAKE_BINARY_DIR}/__nuget/Microsoft.WindowsAppSDK.ML.1.8.2109/lib/native/x64/onnxruntime.lib"
    )
endif()


#if(WIN32)
#    target_link_libraries(${CORE} PUBLIC ${LIB_WINDOWSML})
#endif(WIN32)