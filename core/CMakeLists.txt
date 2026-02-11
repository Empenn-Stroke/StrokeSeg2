add_library(${CORE}
 "utils/animawrapper.h" 
 "utils/animawrapper.cpp" 
 "utils/log.h"
 "utils/log.cpp"
 "utils/NiftiVolume.h" 
 "utils/NiftiVolume.cpp"
 "utils/path.h" 
 "utils/resampling.cpp"
 "utils/resampling.h"
 "utils/str.h"

 "postprocessing/postprocessor.h"
 "postprocessing/postprocessor.cpp"
 "preprocessing/resampling.h"
 "preprocessing/resampling.cpp"
 "preprocessing/brainextraction.h"
 "preprocessing/brainextraction.cpp"
 "preprocessing/preprocessor.h" 
 "preprocessing/preprocessor.cpp" 
 "preprocessing/resampling.cpp" # TODO: remove
 "preprocessing/resampling.h" # TODO: remove

 "managers/configmanager.h" 
 "managers/configmanager.cpp"
 "managers/appconfig.h"
 "managers/appconfig.h"

)


target_compile_features(${CORE} PUBLIC cxx_std_20)
target_include_directories(${CORE} PUBLIC
    ${CMAKE_SOURCE_DIR}/${CORE}/
    ${CMAKE_SOURCE_DIR}/${COMMON}/
)

target_link_libraries(${CORE} PUBLIC
    zlibstatic
    niftiio
    znz
    spdlog
    Qt6::Core
    eigen
)


add_dependencies(${CORE} zlib)

#if(WIN32)
#    target_link_libraries(${CORE} PUBLIC ${LIB_WINDOWSML})
#endif(WIN32)