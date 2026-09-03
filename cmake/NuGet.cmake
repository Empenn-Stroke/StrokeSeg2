# Require CMake version
cmake_minimum_required(VERSION 3.15)

# Download nuget.exe if it does not exist in the build directory
if(NOT EXISTS "${CMAKE_BINARY_DIR}/nuget.exe")
    message(STATUS "Downloading nuget.exe from dist.nuget.org...")
    file(DOWNLOAD "https://dist.nuget.org/win-x86-commandline/latest/nuget.exe" "${CMAKE_BINARY_DIR}/nuget.exe")
endif()

# Locate the downloaded executable
find_program(NUGET_EXE NAMES "nuget.exe" PATHS "${CMAKE_BINARY_DIR}" NO_DEFAULT_PATH)

if(NOT NUGET_EXE)
    message(FATAL_ERROR "nuget.exe not found in ${CMAKE_BINARY_DIR}. Please check your internet connection or permissions.")
endif()

# Function to download a NuGet package via ExternalProject_Add
# package_name: The ID of the NuGet package
# package_version: The exact version to retrieve
# out_package_root: The variable that will hold the path to the extracted package
function(add_nuget_package package_name package_version out_package_root)
    set(package_dest "${CMAKE_BINARY_DIR}/nuget_packages")
    
    # Prefix for ExternalProject internal folders
    set(ep_prefix "${EP_BASE_PATH}/External_Nuget_${NU_PACKAGE_NAME}")

    ExternalProject_Add(
        External_Nuget_${package_name}

        PREFIX "${ep_prefix}"
        
        DOWNLOAD_COMMAND ${NUGET_EXE} install ${package_name} -Version ${package_version} -OutputDirectory "${package_dest}"
        
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
    )

    # Expose the path to the caller scope
    set(${out_package_root} "${package_dest}/${package_name}.${package_version}" PARENT_SCOPE)
endfunction()
