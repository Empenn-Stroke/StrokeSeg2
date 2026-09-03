# SPDX-License-Identifier: AGPL-3.0-or-later

ExternalProject_Add(${ep}
    PREFIX "${EP_BASE_PATH}/${ep}/"
    
    TMP_DIR      "${EP_BASE_PATH}/tmp/${ep}/"
    STAMP_DIR    "${EP_BASE_PATH}/stamp/${ep}/"
    DOWNLOAD_DIR "${EP_BASE_PATH}/download/${ep}/"
    SOURCE_DIR   "${EP_BASE_PATH}/source/${ep}/"
    BINARY_DIR   "${EP_BASE_PATH}/build/${ep}/"
    INSTALL_DIR  "${EP_BASE_PATH}/install/${ep}/"
    LOG_DIR      "${EP_BASE_PATH}/log/${ep}/"
    
    URL "${${ep}_URL}"
    URL_HASH SHA256=${${ep}_HASH}  
    
    CMAKE_GENERATOR ${gen}
    CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM}
    CMAKE_ARGS ${cmake_args}
    CMAKE_CACHE_ARGS ${cmake_cache_args}
    
    DOWNLOAD_NAME     "${${ep}_NAME}"
    PATCH_COMMAND     "${${ep}_PATCH_CMD}"
    CONFIGURE_COMMAND ""  # No configure step
    BUILD_COMMAND     ""  # No build step
    INSTALL_COMMAND   "${${ep}_INSTALL_CMD}"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    STEP_TARGETS download install # Only enable download and install (unzip)
    LOG_DOWNLOAD ON
)

# --- Debugging Messages Block ---
# This block provides a final set of messages to confirm the configuration
# of the core project, which is useful for debugging.
if(${STROKESEG_DEBUG_CMAKE})
    message("******************************")
    message("EP     : ${ep}")
    message("Name   : ${${ep}_NAME}")
    message("Patch  : ${${ep}_PATCH_CMD}")
    message("Config : ")
    message("Build  : ")
    message("Install: ${${ep}_INSTALL_CMD}")
    message("******************************")
endif()
