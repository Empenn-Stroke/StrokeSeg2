cmake_minimum_required(VERSION 3.31)

# --- Policies & System Setup ---
cmake_policy(SET CMP0141 NEW)
cmake_policy(SET CMP0117 NEW)
cmake_policy(SET CMP0174 NEW)
set(CMAKE_SYSTEM_VERSION "10.0.18362.0")

project(strokeseg2-app
    VERSION 0.0.1
    LANGUAGES CXX
)

# --- Configuration des Chemins de Recherche ---
# On dit à CMake de chercher les paquets dans le dossier d'installation du Superprojet
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/bin")
endif()

# --- Setup Qt (Généralement trouvé sur le système) ---
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup()

# --- Recherche des dépendances compilées par le Superprojet ---
# Au lieu de FetchContent, on utilise find_package ou find_library
# car le Superprojet les a déjà buildées pour nous.

# 1. Eigen
find_package(Eigen3 REQUIRED)

# 2. ZLIB
find_package(ZLIB REQUIRED)

# 3. NIFTI (compilé via ExternalProject dans le superprojet)
# Si NIFTI n'exporte pas de config CMake, on le cherche manuellement
find_path(NIFTI_INCLUDE_DIR NAMES nifti1.h PATHS ${CMAKE_PREFIX_PATH}/include)
find_library(NIFTI_LIBRARY NAMES niftiio PATHS ${CMAKE_PREFIX_PATH}/lib)

# 4. QT
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup() # sets some variables to setup qt

# --- NuGet Packages (Gardé ici car spécifique au build MSVC de l'app) ---
include(FetchContent)
FetchContent_Declare(
    NuGetCMakePackage
    GIT_REPOSITORY https://github.com/mschofie/NuGetCMakePackage
    GIT_TAG b13e013481605d134f498fd9fc3a123c416da197
)
FetchContent_MakeAvailable(NuGetCMakePackage)

add_nuget_packages(
    PACKAGES
        Microsoft.Windows.CppWinRT 2.0.250303.1
        Microsoft.ML.OnnxRuntime.DirectML 1.17.1
        Microsoft.Windows.ImplementationLibrary 1.0.250325.1
        Microsoft.WindowsAppSDK.Foundation 1.8.251104000
        Microsoft.WindowsAppSDK.InteractiveExperiences 1.8.251104001
        Microsoft.WindowsAppSDK.ML 1.8.2109
        Microsoft.WindowsAppSDK.Runtime 1.8.251106002
)

find_package(Microsoft.Windows.ImplementationLibrary CONFIG REQUIRED)
find_package(Microsoft.WindowsAppSDK.ML CONFIG REQUIRED)

# --- Definitions & Configuration ---
set(CORE "core")
set(GUI "gui")

# Récupération de ANIMA_ROOT_PATH passé par le Superprojet
if(NOT ANIMA_ROOT_PATH)
    set(ANIMA_ROOT_PATH "./anima" CACHE PATH "Path to anima")
endif()
add_compile_definitions(ANIMA_ROOT_PATH="${ANIMA_ROOT_PATH}")

include(./Configuration.cmake)

# --- Targets ---
add_subdirectory(${CMAKE_SOURCE_DIR}/${CORE})
add_subdirectory(${CMAKE_SOURCE_DIR}/${GUI})
add_subdirectory(${CMAKE_SOURCE_DIR}/test)

# --- DLL Copy Macro (Indispensable sur Windows) ---
macro(post_build_runtime_dll_copy target_name)
    if(WIN32)
        # Note : Dans un Superbuild, TARGET_RUNTIME_DLLS peut ne pas voir 
        # les DLLs des projets externes. On ajoute souvent une copie manuelle 
        # depuis le dossier bin du superprojet si nécessaire.
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${target_name}>
            $<TARGET_FILE_DIR:${target_name}>
            COMMAND_EXPAND_LISTS
        )
    endif()
endmacro()