# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.

# --- Arguments Definition Block ---
# This block sets the arguments passed to the external project's CMake call.
# It defines the installation prefix and a custom prefix name that will be
# used during the build.
set(cmake_args
  ${ep_common_cache_args}
  )
  
set(cmake_cache_args
  -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/bin/${ep}
  -DPREFIX_NAME:STRING=StrokeSeg 
  )

## #############################################################################
## Check if patch has to be applied
## #############################################################################
  
#ep_GeneratePatchCommand(${ep} ${ep}_PATCH_COMMAND ${ep}.patch)

# --- `ExternalProject_Add` Call Block ---
# This block configures the external project for the bootstrapper. It points
# to a local source directory (`CMAKE_SOURCE_DIR}/BootStrap`) instead of a
# remote URL, ensuring that the local code is built and installed correctly.
ExternalProject_Add(${ep}
    GIT_REPOSITORY "https://github.com/madler/zlib.git"
    GIT_TAG v1.3.1.2
    
    PREFIX "${EP_BASE_PATH}/${ep}"
    BINARY_DIR   "${EP_BASE_PATH}/build/${ep}/"  
    
    PATCH_COMMAND ${${ep}_PATCH_COMMAND}
    CMAKE_GENERATOR ${gen}
    CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM}
    CMAKE_ARGS ${cmake_args}
    CMAKE_CACHE_ARGS ${cmake_cache_args}
    
    DEPENDS
)

# --- Debugging Messages Block ---
# This block provides a final set of messages to confirm the configuration
# of the bootstrapper project, which is useful for debugging.
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

## #############################################################################
## Set variable to provide infos about the project
## #############################################################################

ExternalProject_Get_Property(${ep} binary_dir)
ExternalProject_Get_Property(${ep} install_dir)
set(${ep}_ROOT_DIR ${install_dir} PARENT_SCOPE)
set(${ep}_ROOT ${binary_dir} PARENT_SCOPE)
set(${ep}_DIR  ${binary_dir} PARENT_SCOPE)
