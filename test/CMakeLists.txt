enable_testing()

# test = test

# Test program for core library
add_executable(core_test
    # Add source files here. No need for headers declarations as the tests aren't to be included in
    # other files. core_test.cpp is a dummy file, you can change it if you want.
    core_test.cpp
)

# # Test program for gui executable
# add_executable(gui_test
#     # Add source files here. No need for headers declarations as the tests aren't to be included in
#     # other files. gui_test.cpp is a dummy file, you can change it if you want.
#     # gui_test.cpp
# )

target_link_libraries(core_test PRIVATE GTest::gtest_main ${CORE})
# target_link_libraries(gui_test PRIVATE GTest::gtest_main ${GUI})

include(GoogleTest)
gtest_discover_tests(core_test)
# gtest_discover_tests(gui_test)