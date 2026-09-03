# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.

set(NUGET_CONFIG_CONTENT
"<?xml version=\"1.0\" encoding=\"utf-8\"?>
<configuration>
  <config>
    <add key=\"globalPackagesFolder\" value=\"${CMAKE_BINARY_DIR}/nuget_packages/\" />
  </config>
</configuration>
")

# --- Arguments Definition Block ---
# This block sets the arguments passed to the external project's CMake call.
# It defines the installation prefix and a custom prefix name that will be
# used during the build.
set(cmake_args
  ${ep_common_cache_args}
)

set(cmake_cache_args
  -DCMAKE_INSTALL_PREFIX:PATH=${CMAKE_BINARY_DIR}/bin/
  -DPREFIX_NAME:STRING=StrokeSeg
  -DQt6_DIR:PATH=${Qt6_DIR}
  -DZLIB_ROOT:PATH=${CMAKE_BINARY_DIR}/bin/zlib
  -DZLIB_DIR:PATH=${CMAKE_BINARY_DIR}/bin/zlib/lib/cmake/zlib
  -DNIFTI_DIR:PATH=${CMAKE_BINARY_DIR}/bin/Nifti_clib/share/cmake/NIFTI
  -DEigen3_DIR:PATH=${CMAKE_BINARY_DIR}/bin/share/eigen3/cmake
  -DEigen_DIR:PATH=${CMAKE_BINARY_DIR}/bin/share/eigen3/cmake
  -Donnxruntime_DIR:PATH=${CMAKE_BINARY_DIR}/ExtProjs/onnxruntime/src/onnxruntime/lib/cmake/onnxruntime
  -DANIMA_PATH:PATH=${CMAKE_BINARY_DIR}/bin/Program/Anima
  -DDCM2NIIX_PATH:PATH=${CMAKE_BINARY_DIR}/bin/Program/dcm2niix
  "-Dmicrosoft.windows.ai.machinelearning_DIR:PATH=${CMAKE_BINARY_DIR}/nuget_packages/Microsoft.WindowsAppSDK.ML.1.8.2141/build/cmake"
  -DNUGET_ROOT:PATH=${CMAKE_BINARY_DIR}/nuget_packages
  -DCMAKE_PREFIX_PATH:PATH=${CMAKE_BINARY_DIR}/bin
  -DCMAKE_SYSTEM_VERSION:STRING=${CMAKE_SYSTEM_VERSION}
  -DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION:STRING=${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}
  -DNUGET_PACKAGES:PATH=${CMAKE_BINARY_DIR}/packages
  -DGPU:BOOL=${STROKESEG_USE_GPU}
)

## #############################################################################
## Check if patch has to be applied
## #############################################################################
  
#ep_GeneratePatchCommand(${ep} ${ep}_PATCH_COMMAND ${ep}.patch)

get_filename_component(LOCAL_APP_PATH "${CMAKE_SOURCE_DIR}/sources" ABSOLUTE)

if(WIN32)
    set(STROKESEG_DEPS zlib Nifti_clib Anima Eigen) # Sans onnxruntime
else()
    set(STROKESEG_DEPS onnxruntime zlib Nifti_clib Anima Eigen)
endif()

# --- `ExternalProject_Add` Call Block ---
# This block configures the external project for the bootstrapper. It points
# to a local source directory (`CMAKE_SOURCE_DIR}/BootStrap`) instead of a
# remote URL, ensuring that the local code is built and installed correctly.
ExternalProject_Add(${ep}
    PREFIX "${EP_BASE_PATH}/${ep}"
    
    SOURCE_DIR          "${LOCAL_APP_PATH}"
    # We use this method to ensure that the local source code is used for 
    # the build, and it allows us to apply any necessary patches directly 
    # to the local copy. This approach is particularly useful during development, 
    # as it enables us to test changes without needing to push them to a remote 
    # repository first.

    CONFIGURE_HANDLED_BY_BUILD ON

    BINARY_DIR          "${EP_BASE_PATH}/build/${ep}/"  
    
    PATCH_COMMAND       ${${ep}_PATCH_COMMAND}
    CMAKE_GENERATOR     ${gen}
    CMAKE_GENERATOR_PLATFORM ${CMAKE_GENERATOR_PLATFORM}
    CMAKE_ARGS          ${cmake_args}
    CMAKE_CACHE_ARGS    ${cmake_cache_args}
    
    DEPENDS             ${STROKESEG_DEPS}
    INSTALL_COMMAND     ""
)

# 3. On écrit le fichier directement dans le dossier de build (là où sera le .sln)
file(WRITE "${EP_BASE_PATH}/build/${ep}/nuget.config" "${NUGET_CONFIG_CONTENT}")

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
set(${ep}_ROOT ${binary_dir} PARENT_SCOPE)
set(${ep}_DIR  ${binary_dir} PARENT_SCOPE)
