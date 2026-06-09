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
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/bin")
endif()

# --- Setup Qt & Dépendances ---
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent)
qt_standard_project_setup()

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
message(STATUS "Searching for Zlib in: ${Zlib_DIR}")

# 3. NIFTI
find_package(NIFTI REQUIRED)
message(STATUS "Searching for Nifti in: ${NIFTI_DIR}")

# 4. WinML/ONNX Runtime
if (WIN32)
    find_package(microsoft.windows.ai.machinelearning REQUIRED)
    message(STATUS "Searching for Microsoft.Windows.AI.MachineLearning in: ${WINML_DIR}")
    message(STATUS "Build dir: ${CMAKE_BINARY_DIR}")
else ()
    find_package(onnxruntime REQUIRED)
    if(TARGET onnxruntime::onnxruntime)
        get_target_property(ORT_WRONG_INCLUDE_DIR onnxruntime::onnxruntime INTERFACE_INCLUDE_DIRECTORIES)
        get_filename_component(ORT_CORRECT_INCLUDE_DIR "${ORT_WRONG_INCLUDE_DIR}" DIRECTORY)
        set_target_properties(onnxruntime::onnxruntime PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${ORT_CORRECT_INCLUDE_DIR}"
        )
    endif()
endif()

# --- Definitions & Configuration ---
set(CORE "core")
set(GUI "gui")

# [CORRECTION] Gestion propre et conforme de l'arborescence selon l'OS
if(NOT ANIMA_RELATIVE_PATH)
    if(APPLE)
        set(ANIMA_RELATIVE_PATH "." CACHE PATH "Path to anima sur macOS")
    else()
        set(ANIMA_RELATIVE_PATH "./anima" CACHE PATH "Path to anima")
    endif()
endif()
add_compile_definitions(ANIMA_RELATIVE_PATH="${ANIMA_RELATIVE_PATH}")

include(./Configuration.cmake)

qt_add_executable(
        ${PROJECT_NAME}
        MACOSX_BUNDLE
        "gui/resources/StrokeSeg2.icns"
        resources.qrc
        main.cpp
)

if(APPLE)
    # Force la création du Bundle .app au lieu d'un binaire nu
    set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE TRUE)
endif()

if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
    )
endif()

# --- Targets ---
add_subdirectory(${CMAKE_SOURCE_DIR}/${CORE})
add_subdirectory(${CMAKE_SOURCE_DIR}/${GUI})

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${GUI}
)

# =====================================================================
# ÉTAPES POST-BUILD
# =====================================================================

# 1. Déploiement des DLLs (Spécifique Windows)
if(WIN32)    
    set(WINML_DLL_FILES
        "${WINML_BINARY_DIR}/Microsoft.Windows.AI.MachineLearning.dll"
        "${WINML_BINARY_DIR}/onnxruntime.dll"
        "${WINML_DIRECTML_DLL}"
        "${WINML_ONNXRUNTIME_PROVIDERS_SHARED_DLL}"
    )

    set(ZLIB_DLL_DYNAMIC "$<IF:$<CONFIG:Debug>,${ZLIB_ROOT}/bin/zd.dll,${ZLIB_ROOT}/bin/z.dll>")

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${WINML_DLL_FILES}
            "${ZLIB_DLL_DYNAMIC}"
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
        COMMAND_EXPAND_LISTS
        COMMENT "Déploiement des DLLs (Configuration : $<CONFIG>)"
    )
endif()

# 2. Copie d'ANIMA (On le fait AVANT macdeployqt pour qu'il puisse tout signer ensemble)
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

set(MACOS_CHMOD_COMMAND "")
if(APPLE)
    set(MACOS_CHMOD_COMMAND COMMAND chmod +x 
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaApplyTransformSerie"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaConvertImage"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaCreateImage"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaDenseSVFBMRegistration"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaMaskImage"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaN4BiasCorrection"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaPyramidalBMRegistration"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/animaTransformSerieXmlGenerator"
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "DEBUG DESTINATION ANIMA : $<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${ANIMA_FILES_TO_COPY}
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/"
    ${MACOS_CHMOD_COMMAND}
    COMMENT "Copie des outils ANIMA dans le dossier de l'exécutable ($<CONFIG>)"
)

# 3. Outils de Déploiement Qt (Signe aussi ANIMA sur Mac maintenant qu'il est bien placé)
get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
                --no-translations
                --no-system-d3d-compiler
                "$<TARGET_FILE:${PROJECT_NAME}>"
        VERBATIM
        COMMENT "Déploiement Qt (Windows)"
    )
elseif(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/macdeployqt" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
        COMMENT "Déploiement Qt (macOS)"
    )
endif()

# 4. Envoi vers le dossier central du Superprojet
set(PROGRAM_DIR "${CMAKE_BINARY_DIR}/../../../bin/Program/StrokeSeg2")

if(APPLE)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROGRAM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
            "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>"
            "${PROGRAM_DIR}/${PROJECT_NAME}.app"
        COMMENT "Envoi du Bundle .app vers le dossier central du Superprojet"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROGRAM_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different 
            "$<TARGET_FILE:${PROJECT_NAME}>"
            "${PROGRAM_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/anima"
            "${PROGRAM_DIR}/anima"
        COMMENT "Envoi de l'application et d'ANIMA vers le dossier central du Superprojet"
    )
endif()