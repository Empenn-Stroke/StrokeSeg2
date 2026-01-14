cmake_minimum_required(VERSION 3.10...3.31) # support for CXX20

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

add_compile_definitions(ANIMA_ROOT_PATH=${CMAKE_INSTALL_PREFIX}/${ANIMA_ROOT_PATH})

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
find_package(Qt6 REQUIRED COMPONENTS Core Test)
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


# TESTING
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)

# PLATFORM-SPECIFIC
if(WIN32)
    # WINDOWS SDK
    # WindowsML powered by ONNX runtime
    find_library(LIB_WINDOWSML WindowsApp)
endif(WIN32)

# TARGETS ##########################################################################################

add_subdirectory(${CMAKE_SOURCE_DIR}/${CORE})
add_subdirectory(${CMAKE_SOURCE_DIR}/${GUI})
add_subdirectory(${CMAKE_SOURCE_DIR}/test)