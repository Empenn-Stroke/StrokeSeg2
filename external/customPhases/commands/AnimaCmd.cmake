# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: AnimaCmd.cmake
#
# Description: This CMake command module defines the specific actions for the
# Anima external project. As Anima is a pre-compiled binary, this script
# focuses solely on the installation phase, ensuring that the necessary
# executable files are copied to the correct destination.
#
# Main functional blocks:
# 1. Executable List Definition: Lists the specific Anima executables required
#    by the main application.
# 2. Install Command Definition: Constructs a custom installation command that
#    invokes a separate script (`animaInstall.cmake`) to handle the file copying
#    process.

set(_EP Anima)

# Define the list of Anima executables to copy
set(ANIMA_EXECUTABLES
    animaApplyTransformSerie
    animaConvertImage
    animaCreateImage
    animaDenseSVFBMRegistration
    animaMaskImage
    animaN4BiasCorrection
    animaPyramidalBMRegistration
    animaTransformSerieXmlGenerator
)

list(JOIN ANIMA_EXECUTABLES "," ANIMA_EXECUTABLES)

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
    -DANIMA_UNPACK_DIR=${CMAKE_BINARY_DIR}/ExtProjs/source/Anima
    -DANIMA_DEST_DIR=${ANIMA_DEST_DIR}
    -DANIMA_EXECUTABLES=${ANIMA_EXECUTABLES}
    -P
    ${CMAKE_CURRENT_SOURCE_DIR}/customPhases/scripts/AnimaInstall.cmake)

