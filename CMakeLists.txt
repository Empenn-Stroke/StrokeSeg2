cmake_minimum_required(VERSION 3.31) # support for CXX20

cmake_policy(SET CMP0141 NEW) # MSVC debug information format flags are selected by an abstraction.
cmake_policy(SET CMP0117 NEW) # MSVC RTTI flag /GR is not added to CMAKE_CXX_FLAGS by default.
cmake_policy(SET CMP0174 NEW)

set(CMAKE_SYSTEM_VERSION "10.0.18362.0")

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
    string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")

    add_compile_options(/Zm200 /bigobj)

    add_compile_options(/O2 /arch:AVX2 /Oi /Ot /GL)
    add_link_options(/LTCG)
endif()

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    file(WRITE "${CMAKE_BINARY_DIR}/Directory.Build.rsp" "-m -graphBuild:true -p:UseMultiToolTask=true -nodeReuse:false -terminalLogger:auto")
    file(WRITE "${CMAKE_BINARY_DIR}/Directory.Build.props" "<Project></Project>")
    file(WRITE "${CMAKE_BINARY_DIR}/Directory.Build.targets" "<Project></Project>")
endif()

set(ENV{WindowsSdkDir} "C:/Program Files (x86)/Windows Kits/10")

if(NOT EXISTS "$ENV{WindowsSdkDir}/Platforms/UAP/10.0.26100.0/Platform.xml")
    message("$ENV{WindowsSdkDir}/Platforms/UAP/10.0.26100.0/Platform.xml")
    message(FATAL_ERROR "Windows SDK UWP 10.0.26100.0 non trouvé.")
endif()

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(strokeseg2-app
  VERSION 0.0.1
  LANGUAGES CXX
)

include(FetchContent)

# VARS #############################################################################################

set(CORE "core")
set(GUI "gui")
set(COMMON "common")

set(ANIMA_ROOT_PATH ./anima
    CACHE PATH "path to root of anima installation"
)

#add_compile_definitions(ANIMA_ROOT_PATH=${CMAKE_INSTALL_PREFIX}/${ANIMA_ROOT_PATH})
add_compile_definitions(ANIMA_ROOT_PATH=${CMAKE_BINARY_DIR}/${ANIMA_ROOT_PATH})
message(${ANIMA_ROOT_PATH})

# DEPENDENCIES #####################################################################################

# SPDLOG
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
)
FetchContent_MakeAvailable(spdlog)

# EIGEN
FetchContent_Declare(
     eigen
     GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
     GIT_TAG 5.0.0
)
FetchContent_MakeAvailable(eigen)

# QT
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup() # sets some variables to setup qt

# ZLIB
FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG v1.2.13
)
FetchContent_MakeAvailable(zlib)

set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR})
set(ZLIB_LIBRARY zlib)
set(ZLIB_FOUND TRUE)

include_directories(${zlib_SOURCE_DIR})
include_directories(${zlib_BINARY_DIR})


# ---- NIFTI : valeurs NON VIDES ----
set(NIFTI_INSTALL_EXPORT_NAME "NIFTI_SKIP_EXPORT" CACHE STRING "")
set(NIFTI_INSTALL_LIBRARY_DIR "nifti_skip" CACHE PATH "")
set(NIFTI_INSTALL_INCLUDE_DIR "nifti_skip" CACHE PATH "")
set(NIFTI_INSTALL_RUNTIME_DIR "nifti_skip" CACHE PATH "")
set(NIFTI_INSTALL_ARCHIVE_DIR "nifti_skip" CACHE PATH "")
set(NIFTI_BUILD_SHARED_LIBS OFF CACHE BOOL "")

FetchContent_Declare(
  nifti
  GIT_REPOSITORY https://github.com/NIFTI-Imaging/nifti_clib.git
  GIT_TAG master
)
FetchContent_MakeAvailable(nifti)


FetchContent_Declare(
  NuGetCMakePackage
  GIT_REPOSITORY https://github.com/mschofie/NuGetCMakePackage
  GIT_TAG b13e013481605d134f498fd9fc3a123c416da197
)


# Adding specific dependencies depending on the operating system
if(WIN32)
    FetchContent_MakeAvailable(NuGetCMakePackage)

    add_nuget_packages(
        PACKAGES
            Microsoft.Windows.CppWinRT 2.0.250303.1
            Microsoft.ML.OnnxRuntime.DirectML 1.24.4
            Microsoft.Windows.ImplementationLibrary 1.0.250325.1
            Microsoft.WindowsAppSDK.Foundation 1.8.251104000
            Microsoft.WindowsAppSDK.InteractiveExperiences 1.8.251104001
            Microsoft.WindowsAppSDK.ML 1.8.2109
            Microsoft.WindowsAppSDK.Runtime 1.8.251106002
    )

    set(CPPWINRT_VERSION "2.0.250303.1" CACHE STRING "Version of CppWinRT")
    set(CPPWINRT_SYSTEM_VERSION ${CPPWINRT_VERSION} CACHE STRING "System version of CppWinRT")
    set(CPP_WINRT_VERSION ${CPPWINRT_VERSION})
    set(WIN_APP_SDK_VERSION "1.8.251106002")

    set(NUGET_PACKAGES_DIR "${CMAKE_BINARY_DIR}/packages")
    file(MAKE_DIRECTORY "${NUGET_PACKAGES_DIR}")

    find_package(Microsoft.Windows.ImplementationLibrary CONFIG REQUIRED)
    find_package(Microsoft.WindowsAppSDK.ML CONFIG REQUIRED)
else()
    if(APPLE)
        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.24.4/onnxruntime-osx-arm64-1.24.4.tgz")
    else()
        set(ONNX_URL "https://github.com/microsoft/onnxruntime/releases/download/v1.24.4/onnxruntime-linux-x64-gpu-1.24.4.tgz")
    endif()

    FetchContent_Declare(
        onnxruntime
        URL ${ONNX_URL}
    )

    FetchContent_MakeAvailable(onnxruntime)

    include_directories(${onnxruntime_SOURCE_DIR}/include)
    
    add_library(onnxruntime_lib SHARED IMPORTED)
    if(APPLE)
        set_target_properties(onnxruntime_lib PROPERTIES 
            IMPORTED_LOCATION "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.dylib")
    else()
        set_target_properties(onnxruntime_lib PROPERTIES 
            IMPORTED_LOCATION "${onnxruntime_SOURCE_DIR}/lib/libonnxruntime.so")
    endif()
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(DIRECTORY APPEND PROPERTY VS_SOLUTION_ITEMS
    .clangd
    .editorconfig
    .gitignore
    CMakePresets.json
    readme.md
)

include(./Configuration.cmake)


# TARGETS ##########################################################################################

add_subdirectory(${CMAKE_SOURCE_DIR}/${CORE})
add_subdirectory(${CMAKE_SOURCE_DIR}/${GUI})
add_subdirectory(${CMAKE_SOURCE_DIR}/test)

macro(post_build_runtime_dll_copy target_name)
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND "${CMAKE_COMMAND};-E;$<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target_name}>>,copy;$<TARGET_RUNTIME_DLLS:${target_name}>;$<TARGET_FILE_DIR:${target_name}>,true>"
            COMMAND_EXPAND_LISTS
        )
    endif()
endmacro()