# SPDX-License-Identifier: AGPL-3.0-or-later
cmake_minimum_required(VERSION 3.15)

# --- Policies & System Setup ---
cmake_policy(SET CMP0141 NEW)
cmake_policy(SET CMP0117 NEW)
set(CMAKE_SYSTEM_VERSION "10.0.18362.0")

project(strokeseg2-app
    VERSION 0.0.1
    LANGUAGES CXX
)

if(APPLE)
    # Allow loading .dylib from Contents/Frameworks/ and Contents/MacOS/
    set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks" "@executable_path/")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
elseif(UNIX)
    set(CMAKE_INSTALL_RPATH "$ORIGIN")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Search path config ---
if(NOT CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/bin")
endif()

# --- Setup Qt & Dependancies ---
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

# 2. ZLIB
find_package(ZLIB REQUIRED)

# 3. NIFTI
find_package(NIFTI REQUIRED)

# 4. WinML/ONNX Runtime
if (WIN32)
    find_package(microsoft.windows.ai.machinelearning REQUIRED)
else ()
    find_package(onnxruntime REQUIRED)
    if(TARGET onnxruntime::onnxruntime)
        get_target_property(ORT_WRONG_INCLUDE_DIR onnxruntime::onnxruntime INTERFACE_INCLUDE_DIRECTORIES)
        if(ORT_WRONG_INCLUDE_DIR)
            get_filename_component(ORT_CORRECT_INCLUDE_DIR "${ORT_WRONG_INCLUDE_DIR}" DIRECTORY)
            set_target_properties(onnxruntime::onnxruntime PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${ORT_CORRECT_INCLUDE_DIR}"
            )
        endif()
    endif()
endif()

# --- Definitions & Configuration ---
set(CORE "core")
set(GUI "gui")

if(NOT ANIMA_RELATIVE_PATH)
    if(APPLE)
        set(ANIMA_RELATIVE_PATH "." CACHE PATH "Path to anima sur macOS")
    else()
        set(ANIMA_RELATIVE_PATH "./anima" CACHE PATH "Path to anima")
    endif()
endif()
add_compile_definitions(ANIMA_RELATIVE_PATH="${ANIMA_RELATIVE_PATH}")

if(NOT DCM2NIIX_RELATIVE_PATH)
    if(APPLE)
        set(DCM2NIIX_RELATIVE_PATH "." CACHE PATH "Path to dcm2niix sur macOS")
    else()
        set(DCM2NIIX_RELATIVE_PATH "./dcm2niix" CACHE PATH "Path to dcm2niix")
    endif()
endif()
add_compile_definitions(DCM2NIIX_RELATIVE_PATH="${DCM2NIIX_RELATIVE_PATH}")

include(./Configuration.cmake)
set(MAC_ICON_FILE "${CMAKE_CURRENT_SOURCE_DIR}/gui/resources/StrokeSeg2.icns")

qt_add_executable(
        ${PROJECT_NAME}
        MACOSX_BUNDLE
        ${MAC_ICON_FILE}
        resources.qrc
        main.cpp
)

add_compile_definitions(APP_NAME="${PREFIX_NAME}")

if(APPLE)
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
# POST-BUILD STEPS
# =====================================================================

# 1. Deploy DLLs (Windows)
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
        COMMENT "Deployment of DLLs (Configuration : $<CONFIG>)"
    )
endif()

# 2. Copy ANIMA
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
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/"
    COMMAND_EXPAND_LISTS
    COMMENT "Copy of the ANIMA tools"
)

# Explicit application of execution permissions on ANIMA under macOS
if(APPLE)
    foreach(TOOL ${ANIMA_REQUIRED_TOOLS})
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND chmod +x "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${ANIMA_RELATIVE_PATH}/${TOOL}"
        )
    endforeach()
endif()

# 3. Copy dcm2niix
set(DCM2NIIX_TARGET_DIR "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${DCM2NIIX_RELATIVE_PATH}")

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DCM2NIIX_TARGET_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DCM2NIIX_PATH}"
            "${DCM2NIIX_TARGET_DIR}/"
        COMMENT "Copy dcm2niix"
    )
else()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DCM2NIIX_TARGET_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DCM2NIIX_PATH}/dcm2niix"
            "${DCM2NIIX_TARGET_DIR}/"
        COMMENT "Copy of the dcm2niix executable"
    )

    if(APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND chmod +x "${DCM2NIIX_TARGET_DIR}/dcm2niix"
        )
    endif()
endif()

# 4. Qt Deployment & Embarquement of dylib (macOS)
get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
                --no-translations
                --no-system-d3d-compiler
				--compiler-runtime
                "$<TARGET_FILE:${PROJECT_NAME}>"
        VERBATIM
        COMMENT "Deployment of Qt (Windows)"
    )	

elseif(APPLE)
    # macdeployqt integrates Qt, adding RPATHs and including the necessary frameworks
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/macdeployqt" "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>" -always-overwrite
        COMMENT "Deployment of Qt (macOS)"
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND chmod -R +x "$<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/MacOS/"
        COMMENT "Application finale des droits d'exécution sur Contents/MacOS"
    )
endif()

# 5. Copy to superproject folder
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
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "$<TARGET_FILE_DIR:${PROJECT_NAME}>/dcm2niix"
            "${PROGRAM_DIR}/dcm2niix"
        COMMENT "Envoi de l'application vers le dossier central du Superprojet"
    )
endif()