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

if(MSVC)
    #string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    #string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    #string(REPLACE "/ZI" "/Zi" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    #add_compile_options(/Zm200 /bigobj)
    #add_compile_options(/O2 /arch:AVX2 /Oi /Ot /GL)
    #add_link_options(/LTCG)
endif()

if(CMAKE_GENERATOR MATCHES "^Visual Studio")
    file(WRITE "${CMAKE_BINARY_DIR}/Directory.Build.rsp" "-m -graphBuild:true -p:UseMultiToolTask=true -nodeReuse:false -terminalLogger:auto")
    file(WRITE "${CMAKE_BINARY_DIR}/Directory.Build.props" "<Project></Project>")
    file(WRITE "${CMAKE_BINARY_DIR}/Directory.Build.targets" "<Project></Project>")
endif()

# 1. Eigen
set(EIGEN_INSTALL_DIR "${CMAKE_BINARY_DIR}/../../bin/eigen")
list(APPEND CMAKE_PREFIX_PATH 
    "${EIGEN_INSTALL_DIR}"
    "${CMAKE_BINARY_DIR}/../../bin"
)
find_package(Eigen3 REQUIRED)
message(STATUS "Searching for Eigen3 in: ${CMAKE_PREFIX_PATH}")

# 2. ZLIB
get_filename_component(OUT_DIR "${CMAKE_BINARY_DIR}/../../.." ABSOLUTE)
set(ZLIB_LIB_DIR "${OUT_DIR}/bin/zlib/lib")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ZLIB_LIBRARY "${ZLIB_LIB_DIR}/zsd.lib")
else()
    set(ZLIB_LIBRARY "${ZLIB_LIB_DIR}/zd.lib")
endif()

# 3. NIFTI (compilé via ExternalProject dans le superprojet)
# Si NIFTI n'exporte pas de config CMake, on le cherche manuellement
get_filename_component(OUT_DIR "${CMAKE_BINARY_DIR}/../../.." ABSOLUTE)
set(NIFTI_INSTALL_ROOT "${OUT_DIR}/bin/Nifti_clib")
set(NIFTI_INCLUDE_DIR "${NIFTI_INSTALL_ROOT}/include/nifti")
set(NIFTI_LIBS 
    "${NIFTI_INSTALL_ROOT}/lib/niftiio.lib"
    "${NIFTI_INSTALL_ROOT}/lib/znz.lib"
)
# On vérifie si les chemins existent pour aider au debug
message(STATUS "Checking Nifti in: ${NIFTI_INCLUDE_DIR}")

# 4. QT
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup() # sets some variables to setup qt

#5. ANIMA
set(ANIMA_REQUIRED_TOOLS 
    "animaApplyTransformSerie"
    "animaConvertImage"
    "animaCreateImage"
    "animaDenseSVFBMRegistration"
    "animaMaskImage"
    "animaN4BiasCorrection"
    "animaPyramidalBMRegistration"
    "animaTransformSerieXmlGenerator"
)

set(ANIMA_FILES_TO_COPY "")
foreach(TOOL ${ANIMA_REQUIRED_TOOLS})
    if(WIN32)
        set(BIN_PATH "${anima_SOURCE_DIR}/${TOOL}.exe")
    else()
        set(BIN_PATH "${anima_SOURCE_DIR}/${TOOL}")
    endif()
        list(APPEND ANIMA_FILES_TO_COPY "${BIN_PATH}")
endforeach()

set(ANIMA_RUNTIME_DESTINATION "${CMAKE_BINARY_DIR}/${ANIMA_REL_PATH}")
file(MAKE_DIRECTORY "${ANIMA_RUNTIME_DESTINATION}")
foreach(FILE_PATH ${ANIMA_FILES_TO_COPY})
    if(EXISTS "${FILE_PATH}")
        file(COPY "${FILE_PATH}" DESTINATION "${ANIMA_RUNTIME_DESTINATION}")
    else()
        message(WARNING "ANIMA binary not found at path : ${FILE_PATH}")
    endif()
endforeach()

# --- Definitions & Configuration ---
set(CORE "core")
set(GUI "gui")

# Récupération de ANIMA_ROOT_PATH passé par le Superprojet
if(NOT ANIMA_ROOT_PATH)
    set(ANIMA_ROOT_PATH "./anima" CACHE PATH "Path to anima")
endif()
add_compile_definitions(ANIMA_ROOT_PATH="${ANIMA_ROOT_PATH}")

include(./Configuration.cmake)

qt_add_executable(${PROJECT_NAME})



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