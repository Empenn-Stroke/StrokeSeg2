add_library(${CORE}
 "utils/animawrapper.h" 
 "utils/animawrapper.cpp" 
 "utils/path.h" 
 "utils/str.h"
 "preprocessing/resampling.h"
 "preprocessing/resampling.cpp"
 "preprocessing/brainextraction.h"
 "preprocessing/brainextraction.cpp"
 "managers/configmanager.h" 
 "managers/configmanager.cpp"
 "managers/appconfig.h"
 "managers/appconfig.h")

target_compile_features(${CORE} PUBLIC cxx_std_20)
target_include_directories(${CORE} PUBLIC
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
)

target_link_libraries(${CORE} PUBLIC
    spdlog
    #eigen
    Qt6::Core
)

#if(WIN32)
#    target_link_libraries(${CORE} PUBLIC ${LIB_WINDOWSML})
#endif(WIN32)