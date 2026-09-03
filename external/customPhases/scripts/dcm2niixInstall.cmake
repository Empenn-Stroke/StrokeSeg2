# SPDX-License-Identifier: AGPL-3.0-or-later
# Fichier: dcm2niixInstall.cmake

file(MAKE_DIRECTORY "${DCM2NIIX_DEST_DIR}")

string(REPLACE "," ";" DCM2NIIX_EXECUTABLES ${DCM2NIIX_EXECUTABLES})

foreach(executable ${DCM2NIIX_EXECUTABLES})
    file(GLOB actual_source_file "${DCM2NIIX_UNPACK_DIR}/${executable}*")
    if(NOT actual_source_file)
        message(WARNING "dcm2niix executable '${executable}' not found at '${DCM2NIIX_UNPACK_DIR}/${executable}*'. Skipping copy.")
        continue()
    endif()
    message(STATUS "Copying ${actual_source_file} to ${DCM2NIIX_DEST_DIR}/")
    file(COPY "${actual_source_file}" DESTINATION "${DCM2NIIX_DEST_DIR}/")
endforeach()

if(WIN32)
    file(GLOB extra_dlls "${DCM2NIIX_UNPACK_DIR}/*.dll")
    if(extra_dlls)
        foreach(dll ${extra_dlls})
            message(STATUS "Copying companion DLL: ${dll} to ${DCM2NIIX_DEST_DIR}/")
            file(COPY "${dll}" DESTINATION "${DCM2NIIX_DEST_DIR}/")
        endforeach()
    else()
        message(STATUS "No companion DLLs found in ${DCM2NIIX_UNPACK_DIR}. Skipping DLL copy.")
    endif()
endif()
