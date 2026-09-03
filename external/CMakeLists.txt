include(ExternalProject)

if(WIN32)
    include(../cmake/NuGet.cmake)
endif()

set(EP_BASE_PATH ${CMAKE_BINARY_DIR}/ExtProjs)

# --- Generator Selection Block ---
# This block determines which CMake generator should be used for the external
# projects. It checks for a secondary generator (`CMAKE_EXTRA_GENERATOR`) and
# falls back to the primary one (`CMAKE_GENERATOR`) if no extra generator is specified.
if(CMAKE_EXTRA_GENERATOR)
    set(gen "${CMAKE_EXTRA_GENERATOR} -G ${CMAKE_GENERATOR}")
else()
    set(gen "${CMAKE_GENERATOR}")
endif()

# --- Custom Phase Inclusion Block ---
# This block dynamically finds and includes custom command scripts located
# in a 'customPhases/commands' subdirectory. This allows for flexible and
# reusable build steps that can be applied to different external projects.
file(GLOB custumPhases "${CMAKE_CURRENT_SOURCE_DIR}/customPhases/commands/*.cmake")
foreach(custumPhase ${custumPhases})
    message(STATUS "Including custom command from: ${custumPhase}")
    include(${custumPhase})
endforeach()

message(STATUS " ")

## ##############################################################################
## Add modules
## ##############################################################################
set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/customPhases/tools
    ${CMAKE_MODULE_PATH}
    )

## ##############################################################################
## Include common configuration steps
## ##############################################################################
# Correction ici : On inclut les fichiers qui existent réellement sur ton disque
include("${CMAKE_CURRENT_SOURCE_DIR}/customPhases/tools/EP_CheckEnvironment.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/customPhases/tools/EP_GeneratePatchCommand.cmake")



# --- Module Inclusion and Dependency Management Block ---
# This block iterates through all CMake modules in the current directory.
# For each module found, it includes the file, which in turn defines an
# `ExternalProject_Add` call. The name of the project is added to a list
# of dependencies (`ALL_DEPENDENCIES`) to be used later in the main `CMakeLists.txt`.


# Platform-specific dependencies are added conditionally based on the target operating system. 
# This ensures that only the relevant external projects are included for each platform, optimizing the build process and avoiding unnecessary dependencies.
if(WIN32)
    list(APPEND externalProjectsList
        ImplementationLibrary
        CppWinRT
        WindowsAppSDK
    )
elseif(APPLE OR LINUX)
    list(APPEND externalProjectsList
        onnxruntime
    )
endif()

list(APPEND externalProjectsList
  zlib 
  Nifti_clib 
  Anima
  Eigen
  dcm2niix
  StrokeSeg2
)

foreach(ep ${externalProjectsList})
        message(STATUS "Including module: ${ep}")
        include(${CMAKE_CURRENT_SOURCE_DIR}/${ep}.cmake) # Include the CMake external-project module
        list(APPEND ALL_DEPENDENCIES ${ep})
endforeach()

message(STATUS " ")
set(ALL_DEPENDENCIES ${ALL_DEPENDENCIES} PARENT_SCOPE)




















































# Post-build step for NuGetCMakePackage: create a CMakeLists.txt that uses the installed NuGetCMakePackage
# file(WRITE "${EP_BASE_PATH}/source/NuGetCMakePackage/CMakeLists.txt" 
#    "cmake_minimum_required(VERSION 3.15)
#     project(NuGetCMakePackage NONE)
#     install(FILES \"${CMAKE_CURRENT_SOURCE_DIR}/NuGetCMakePackage.cmake\" 
#             DESTINATION . 
#             RENAME NuGetCMakePackageConfig.cmake)
# ")
# 
# file(WRITE "${EP_BASE_PATH}/source/NuGetDeps/CMakeLists.txt" 
#    "cmake_minimum_required(VERSION 3.15)
#     project(NuGetInstallers NONE)
#     
#     # --- FONCTION DE TÉLÉCHARGEMENT DIRECTE (SANS DÉPENDANCE EXTERNE) ---
#     function(install_simple_nuget name version root)
#         set(out_dir \"\${root}/\${name}.\${version}\")
#         if(NOT EXISTS \"\${out_dir}\")
#             message(STATUS \"Downloading NuGet: \${name} (\${version})...\")
#             # Téléchargement de nuget.exe si manquant
#             if(NOT EXISTS \"\${CMAKE_BINARY_DIR}/nuget.exe\")
#                 file(DOWNLOAD https://dist.nuget.org/win-x86-commandline/latest/nuget.exe \"\${CMAKE_BINARY_DIR}/nuget.exe\")
#             endif()
#             # Installation
#             execute_process(
#                 COMMAND \"\${CMAKE_BINARY_DIR}/nuget.exe\" install \${name} -Version \${version} -OutputDirectory \${root}
#                 RESULT_VARIABLE res
#             )
#             if(NOT res EQUAL 0)
#                 message(FATAL_ERROR \"Failed to install \${name}\")
#             endif()
#         endif()
#     endfunction()
# 
#     # --- MACRO COMPATIBLE AVEC TON CODE ---
#     macro(add_nuget_packages)
#         set(root \"\${CMAKE_BINARY_DIR}/__nuget\")
#         set(pkgs \${ARGV})
#         list(REMOVE_ITEM pkgs PACKAGES)
#         while(pkgs)
#             list(POP_FRONT pkgs name)
#             list(POP_FRONT pkgs ver)
#             install_simple_nuget(\${name} \${ver} \${root})
#             # On définit les variables pour que find_package fonctionne
#             set(\${name}_DIR \"\${root}/\${name}.\${ver}/build/native\" CACHE PATH \"\" FORCE)
#             # Certains packages utilisent build/cmake
#             if(NOT EXISTS \"\${\${name}_DIR}\")
#                 set(\${name}_DIR \"\${root}/\${name}.\${ver}/build/cmake\" CACHE PATH \"\" FORCE)
#             endif()
#         endwhile()
#     endmacro()
# 
#     # --- EXÉCUTION ---
#     add_nuget_packages(
#         PACKAGES
#             Microsoft.Windows.CppWinRT 2.0.250303.1
#             Microsoft.Windows.ImplementationLibrary 1.0.250325.1
#             Microsoft.WindowsAppSDK 1.8.260317003
#             Microsoft.ML.OnnxRuntime.DirectML 1.17.1
#     )
# ")
