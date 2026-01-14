#enable_testing()
#
## test = test
#
## Test program for core library
#add_executable(core_test
#    # Add source files here. No need for headers declarations as the tests aren't to be included in
#    # other files. core_test.cpp is a dummy file, you can change it if you want.
#    core_test.cpp
#)
#
## # Test program for gui executable
## add_executable(gui_test
##     # Add source files here. No need for headers declarations as the tests aren't to be included in
##     # other files. gui_test.cpp is a dummy file, you can change it if you want.
##     # gui_test.cpp
## )
#
#target_link_libraries(core_test PRIVATE GTest::gtest_main ${CORE})
## target_link_libraries(gui_test PRIVATE GTest::gtest_main ${GUI})
#
#include(GoogleTest)
#gtest_discover_tests(core_test)
## gtest_discover_tests(gui_test)

enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)


find_package(Qt6 REQUIRED COMPONENTS Test)

message(STATUS "Qt6::Test = ${Qt6Test_FOUND}")

# --------------------------
# Test : NiftiVolume
# --------------------------
qt_add_executable(test_niftivolume
    test_niftivolume.cpp
)

target_link_libraries(test_niftivolume PRIVATE
    Qt6::Test
    ${CORE}
)

add_test(
    NAME test_niftivolume
    COMMAND test_niftivolume
)

# --------------------------
# Test : AnimaWrapper
# --------------------------
qt_add_executable(test_animawrapper
    test_animawrapper.cpp
)

target_link_libraries(test_animawrapper PRIVATE
    Qt6::Test
    ${CORE}
)

add_test(
    NAME test_animawrapper
    COMMAND test_animawrapper
)


# --------------------------
# Test : Preprocessor
# --------------------------
qt_add_executable(test_preprocessor
    test_preprocessor.cpp
)

target_link_libraries(test_preprocessor PRIVATE
    Qt6::Test
    ${CORE}
)

add_test(
    NAME test_preprocessor
    COMMAND test_preprocessor
)


target_compile_definitions(test_niftivolume PRIVATE
    TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data"
)

