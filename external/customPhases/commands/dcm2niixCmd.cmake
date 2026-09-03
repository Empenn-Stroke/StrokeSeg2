# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: dcm2niixCmd.cmake
#
# Description: This CMake command module defines the specific actions for the
# dcm2niix external project. As dcm2niix is a pre-compiled binary, this script
# focuses solely on the installation phase, ensuring that the necessary
# executable files are copied to the correct destination.
#
# Main functional blocks:
# 1. Executable List Definition: Lists the specific dcm2niix executables required
#    by the main application.
# 2. Install Command Definition: Constructs a custom installation command that
#    invokes a separate script (`dcm2niixInstall.cmake`) to handle the file copying
#    process.

set(_EP dcm2niix)

set(DCM2NIIX_EXECUTABLES dcm2niix)

###############################################################################
## DOWNLOAD COMMAND
###############################################################################



###############################################################################
## PATCH COMMAND
###############################################################################



###############################################################################
## BUILD COMMAND
###############################################################################    



###############################################################################
## INSTALL COMMAND
###############################################################################
set(${_EP}_INSTALL_CMD  
    ${CMAKE_COMMAND}
    -DDCM2NIIX_UNPACK_DIR=${CMAKE_BINARY_DIR}/ExtProjs/source/dcm2niix
    -DDCM2NIIX_DEST_DIR=${DCM2NIIX_DEST_DIR}
    -DDCM2NIIX_EXECUTABLES=${DCM2NIIX_EXECUTABLES}
    -P
    ${CMAKE_CURRENT_SOURCE_DIR}/customPhases/scripts/dcm2niixInstall.cmake
)
