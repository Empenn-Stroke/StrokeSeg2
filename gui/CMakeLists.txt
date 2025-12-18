find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(${PROJECT_NAME}
    main.cpp
    mainWindow.h
    mainWindow.cpp
    guideWindow.h
    guideWindow.cpp
    warningWindow.h
    warningWindow.cpp
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/${GUI}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(WIN32)

    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QT_QMAKE_EXECUTABLE}" DIRECTORY)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "${QT_BIN_DIR}/windeployqt.exe"
            --no-translations
            --no-system-d3d-compiler
            \"$<TARGET_FILE:${PROJECT_NAME}>\"
    )
endif()