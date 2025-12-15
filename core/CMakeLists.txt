add_library(${CORE}
  # this dummy file is only here as placeholder and can be removed
  dummy.cpp
 "utils/animawrapper.cpp" "utils/animawrapper.h" "utils/path.h" "utils/str.h")

target_compile_features(${CORE} PUBLIC cxx_std_20)
target_include_directories(${CORE} PRIVATE
    ${CMAKE_SOURCE_DIR}/${CORE}/
)

target_link_libraries(${CORE} PUBLIC
    spdlog
    eigen
    Qt6::Core
)

if(WIN32)
    target_link_libraries(${CORE} PUBLIC ${LIB_WINDOWSML})
endif(WIN32)