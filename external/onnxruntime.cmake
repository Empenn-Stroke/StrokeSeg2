set(ORT_INSTALL_DIR "${EP_BASE_PATH}/install/${ep}/")

if(UNIX AND NOT APPLE)
    set(ONNX_PATCH_CMD 
        "${CMAKE_COMMAND}" -E make_directory "<SOURCE_DIR>/lib64" 
        COMMAND "${CMAKE_COMMAND}" -E create_symlink ../lib/libonnxruntime.so.1.24.4 "<SOURCE_DIR>/lib64/libonnxruntime.so.1.24.4"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink ../lib/libonnxruntime.so.1.24.4 "<SOURCE_DIR>/lib64/libonnxruntime.so.1"
    )
else()
    set(ONNX_PATCH_CMD "${CMAKE_COMMAND}" -E echo "No patch required for Mac")
endif()

set(ONNX_REAL_SRC_DIR "${CMAKE_BINARY_DIR}/ExtProjs/onnxruntime/src/onnxruntime")

ExternalProject_Add(${ep}
    PREFIX "${EP_BASE_PATH}/${ep}/"
    INSTALL_DIR  "${ORT_INSTALL_DIR}"
    URL "${${ep}_URL}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    
    INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory "${ONNX_REAL_SRC_DIR}" "<INSTALL_DIR>"
    PATCH_COMMAND ${ONNX_PATCH_CMD}
)

set(ONNX_INCLUDE_DIR "${ORT_INSTALL_DIR}/include" CACHE PATH "" FORCE)

if(APPLE)
    set(ONNX_LIB "${ORT_INSTALL_DIR}/lib/libonnxruntime.1.24.4.dylib" CACHE FILEPATH "" FORCE)
else()
    set(ONNX_LIB "${ORT_INSTALL_DIR}/lib/libonnxruntime.so" CACHE FILEPATH "" FORCE)
endif()

add_library(onnxruntime::Core SHARED IMPORTED GLOBAL)
set_target_properties(onnxruntime::Core PROPERTIES
    IMPORTED_LOCATION "${ONNX_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${ONNX_INCLUDE_DIR}"
)
add_dependencies(onnxruntime::Core ${ep})