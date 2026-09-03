# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: AnimaInstall.cmake
#
# Description: This script handles the installation of the Anima executables.
# It takes the source and destination directories as input and copies the
# specified list of executables to the installation location.
#
# Main functional blocks:
# 1. Directory Creation: Ensures the destination directory exists.
# 2. Executable Copying Loop: Iterates through the list of executables,
#    locates each one, and copies it to the destination, providing a warning
#    if a file is not found.

# ANIMA_UNPACK_DIR
# ANIMA_DEST_DIR
# ANIMA_EXECUTABLES

file(MAKE_DIRECTORY "${ANIMA_DEST_DIR}")

# The list of executables is passed as a comma-separated string,
# so we convert it back to a list.
string(REPLACE "," ";" ANIMA_EXECUTABLES ${ANIMA_EXECUTABLES})
message("${ANIMA_EXECUTABLES}")
foreach(executable ${ANIMA_EXECUTABLES})
    # Use glob for potential extensions like .exe
    file(GLOB actual_source_file "${ANIMA_UNPACK_DIR}/${executable}*")
    if(NOT actual_source_file)
        message(WARNING "Anima executable '${executable}' not found at '${ANIMA_UNPACK_DIR}/${executable}*'. Skipping copy.")
        continue()
    endif()
    message(STATUS "Copying ${actual_source_file} to ${ANIMA_DEST_DIR}/")
    file(COPY "${actual_source_file}" DESTINATION "${ANIMA_DEST_DIR}/")
endforeach()
