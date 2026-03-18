enable_testing()
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Test)

# --------------------------
# Macro pour créer un test facilement
# --------------------------
macro(add_qt_test TEST_NAME TEST_SRC)
    qt_add_executable(${TEST_NAME} ${TEST_SRC})

    target_link_libraries(${TEST_NAME} PRIVATE
        Qt6::Test
        ${CORE}
    )

    add_test(
        NAME ${TEST_NAME}
        COMMAND ${TEST_NAME}
    )

    target_compile_definitions(${TEST_NAME} PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
    )
endmacro()

# --------------------------
# Tests
# --------------------------

# Test NiftiVolume
#add_qt_test(test_niftivolume test_niftivolume.cpp)

# Test Resampling
#add_qt_test(test_resampling test_resampling.cpp)

# Test AnimaWrapper :
#add_qt_test(test_animawrapper test_animawrapper.cpp)

# Test BrainExtraction :
#add_qt_test(test_brainextraction test_brainextraction.cpp)

# Test Preprocessor :
#add_qt_test(test_preprocessor test_preprocessor.cpp)

# Test Inference :
add_qt_test(test_inference test_inference.cpp)