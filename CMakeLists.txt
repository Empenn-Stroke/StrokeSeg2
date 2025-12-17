cmake_minimum_required(VERSION 3.10...3.31) # support for CXX20

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
#set(CACHE{ANIMA_ROOT_PATH} TYPE PATH HELP "path to root of anima installation" VALUE ./anima)

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
# FetchContent_Declare(
#     eigen
#     GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
#     GIT_TAG 5.0.0
# )
# FetchContent_MakeAvailable(eigen)

# QT
find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup() # sets some variables to setup qt

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