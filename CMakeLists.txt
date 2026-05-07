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

add_compile_definitions(ANIMA_RELATIVE_PATH="Anima")

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
find_package(Eigen3 REQUIRED)
message(STATUS "Searching for Eigen3 in: ${Eigen3_DIR}")

# 2. ZLIB
find_package(ZLIB REQUIRED)
message(STATUS "Searching for Zlib in: ${ZLIB_DIR}")

# 3. NIFTI
find_package(NIFTI REQUIRED)
message(STATUS "Searching for Nifti in: ${NIFTI_DIR}")

# 4. QT
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup() # sets some variables to setup qt

# 5. WinML
find_package(microsoft.windows.ai.machinelearning REQUIRED)
message(STATUS "Searching for Microsoft.Windows.AI.MachineLearning in: ${WINML_DIR}")
message(STATUS "Build dir" ${CMAKE_BINARY_DIR})

# --- Definitions & Configuration ---
set(CORE "core")
set(GUI "gui")

# Récupération de ANIMA_ROOT_PATH passé par le Superprojet
if(NOT ANIMA_ROOT_PATH)
    set(ANIMA_ROOT_PATH "./anima" CACHE PATH "Path to anima")
endif()
add_compile_definitions(ANIMA_ROOT_PATH="${ANIMA_ROOT_PATH}")

include(./Configuration.cmake)

qt_add_executable(
        ${PROJECT_NAME}
        resources.qrc
        main.cpp
)

target_sources(${PROJECT_NAME} PRIVATE "main.cpp")

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()


# --- Targets ---
add_subdirectory(${CMAKE_SOURCE_DIR}/${CORE})
add_subdirectory(${CMAKE_SOURCE_DIR}/${GUI})
#add_subdirectory(${CMAKE_SOURCE_DIR}/test)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${GUI}
)

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
        set(BIN_PATH "${ANIMA_PATH}/${TOOL}.exe")
    else()
        set(BIN_PATH "${ANIMA_PATH}/${TOOL}")
    endif()
    list(APPEND ANIMA_FILES_TO_COPY "${BIN_PATH}")
endforeach()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ANIMA_FILES_TO_COPY}
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}"
    COMMENT "Copie des outils ANIMA dans le dossier de l'exécutable ($<CONFIG>)"
)

# WinML / Zlib DLLs deployment
if(WIN32)    
    set(WINML_DLL_FILES
        "${WINML_BINARY_DIR}/Microsoft.Windows.AI.MachineLearning.dll"
        "${WINML_BINARY_DIR}/onnxruntime.dll"
        "${WINML_DIRECTML_DLL}"
        "${WINML_ONNXRUNTIME_PROVIDERS_SHARED_DLL}"
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(ZLIB_DLL "${ZLIB_ROOT}/bin/zd.dll")
    else()
        set(ZLIB_DLL "${ZLIB_ROOT}/bin/z.dll")
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WINML_DLL_FILES}
            ${ZLIB_DLL}
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
        COMMENT "Déploiement des DLLs"
    )
endif()

# Qt deployment

get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

if(WIN32) 
    set(QT_DEPLOY_BIN "${QT_BIN_DIR}/windeployqt.exe")
elseif(APPLE)
    set(QT_DEPLOY_BIN "${QT_BIN_DIR}/macdeployqt")
else()
    set(QT_DEPLOY_BIN "${QT_BIN_DIR}/linuxdeployqt")
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_DEPLOY_BIN}"
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:${PROJECT_NAME}>"
        VERBATIM
)

set(PROGRAM_DIR "${CMAKE_BINARY_DIR}/../../../bin/Program/StrokeSeg2")

make_directory(${PROGRAM_DIR})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory 
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>" 
        "${PROGRAM_DIR}"
    COMMENT "Envoi de l'application vers le dossier central du Superprojet"
)
