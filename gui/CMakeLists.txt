qt_add_executable(${GUI}
    main.cpp
)

target_compile_features(${GUI} PUBLIC cxx_std_20)
target_include_directories(${GUI} PRIVATE
    ${CMAKE_SOURCE_DIR}/${GUI}/
)

target_link_libraries(${GUI} PUBLIC
    Qt6::Core
)