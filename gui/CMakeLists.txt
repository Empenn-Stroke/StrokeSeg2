# Add the GUI files to the executable defined in the root
add_library(${GUI} STATIC
    "views/mainWindow.h"
    "views/mainWindow.cpp"
    "views/guideWindow.h"
    "views/guideWindow.cpp"
    "views/aboutWindow.h"
    "views/aboutWindow.cpp"
    "views/warningWindow.h"
    "views/warningWindow.cpp"
    "models/modelManager.h"
    "models/modelManager.cpp"
    "workers/pipelineWorker.h"
    "workers/pipelineWorker.cpp"
)

set_target_properties(${GUI} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Handle the icon if it's in this folder
if (WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/appicon.rc")
    target_sources(${PROJECT_NAME} PRIVATE resources/appicon.rc)
endif()

# Requirements for the app
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)


target_include_directories(${GUI} 
PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/views
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/workers

PRIVATE
    ${CMAKE_SOURCE_DIR}/${COMMON}/
    ${CMAKE_SOURCE_DIR}/${CORE}/
)

target_link_libraries(${GUI} PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Concurrent
    ${CORE}
)