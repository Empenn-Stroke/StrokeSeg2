set(cmake_cache_args
  -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/bin/
  -DPREFIX_NAME:STRING=StrokeSeg
  -DBUILD_TESTING:BOOL=OFF
  -DEIGEN_BUILD_DOC:BOOL=OFF
  #-DCMAKE_CXX_FLAGS:STRING=/bigobj /EHsc
  #-DCMAKE_C_FLAGS:STRING=/bigobj
  -DEIGEN_BUILD_LAPACK:BOOL=OFF
  -DEIGEN_BUILD_BLAS:BOOL=OFF
  -DBUILD_TESTING:BOOL=OFF
)

ExternalProject_Add(${ep}
    GIT_REPOSITORY "${${ep}_URL}"
    GIT_TAG        5.0.1

    PREFIX "${EP_BASE_PATH}/${ep}"

    TMP_DIR      "${EP_BASE_PATH}/tmp/${ep}/"
    STAMP_DIR    "${EP_BASE_PATH}/stamp/${ep}/"
    DOWNLOAD_DIR "${EP_BASE_PATH}/download/${ep}/"
    SOURCE_DIR   "${EP_BASE_PATH}/source/${ep}/"
    BINARY_DIR   "${EP_BASE_PATH}/build/${ep}/"
    INSTALL_DIR  "${EP_BASE_PATH}/install/${ep}/"
    LOG_DIR      "${EP_BASE_PATH}/log/${ep}/"
    
    CMAKE_GENERATOR ${gen}
    CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM}
    CMAKE_ARGS ${cmake_args}
    CMAKE_CACHE_ARGS ${cmake_cache_args}

    #INSTALL_COMMAND ${CMAKE_COMMAND} --build . --target install
    
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    LOG_DOWNLOAD ON
)

ExternalProject_Get_Property(${ep} binary_dir)
set(Eigen3_ROOT ${binary_dir} PARENT_SCOPE)
set(Eigen3_DIR  ${binary_dir} PARENT_SCOPE)