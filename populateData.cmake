# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: populateData.cmake
#
# Description: This script is responsible for populating the application's
# data directories with necessary files, including ONNX models and medical
# image atlases. It defines custom targets to handle the copying of user-
# specified models and the downloading of pre-defined atlas files, ensuring
# the application has all the data it needs to function.
#
# Main functional blocks:
# 1. Model Handling: Checks for the existence of user-specified ONNX models
#    (mono and bi modal) and sets up copy commands to move them to the
#    correct destination. It also provides informative messages or errors
#    if a model path is not found.
# 2. Atlas Downloading: Configures commands to download the reference T1 atlas
#    and the brain mask from a remote URL. It uses a separate CMake script
#    for the download process.
# 3. Custom Target Creation: Defines two distinct custom targets:
#    `PopulateDataAtlas` and `PopulateDataModels`. These targets encapsulate
#    the data population logic, making the build process modular and easy to
#    manage.



###############################################################################
##  Models Handling Block
###############################################################################
# This block first creates the destination directory for ONNX models. It then
# checks if the user has provided valid paths for the mono and bi-modal ONNX
# models. If a path exists, it sets up a copy command. If not, it provides
# an informative message to the user.
file(MAKE_DIRECTORY ${ONNX_MODELS_DEST_DIR})


set(ONNX_MODELS_MONO "" CACHE FILEPATH "Model weight in ONNX format for the mono modal (T1) application.")
set(ONNX_MODELS_BI   "" CACHE FILEPATH "Model weight in ONNX format for the bi modal (T1+Flair) application.")
set(ONNX_MODELS_MONO_MANIFEST "" CACHE FILEPATH "Manifest file for the mono modal (T1) application.")
set(ONNX_MODELS_BI_MANIFEST   "" CACHE FILEPATH "Manifest file for the bi modal (T1+Flair) application.")
 
set(ONNX_MODELS_MONO_ONNX_COPY_CMD "")
set(ONNX_MODELS_MONO_MANIFEST_COPY_CMD "")
set(ONNX_MODELS_BI_ONNX_COPY_CMD "")
set(ONNX_MODELS_BI_MANIFEST_COPY_CMD "")
 
### MONO MODEL — .onnx
if(EXISTS "${ONNX_MODELS_MONO}")
    message(STATUS "Copying mono ONNX model from: ${ONNX_MODELS_MONO}")
    set(ONNX_MODELS_MONO_ONNX_COPY_CMD
        ${CMAKE_COMMAND}
        -E
        copy
        "${ONNX_MODELS_MONO}"
        "${ONNX_MODELS_DEST_DIR}/Monomodal.onnx")
else()
    if("${ONNX_MODELS_MONO}" STREQUAL "")
        message("Mono ONNX model path are not specified. Application will be generate without Mono-model.")
    else()
        message(SEND_ERROR "Mono ONNX model path does not exist or is not a directory: ${ONNX_MODELS_MONO}")
        set(ONNX_MODELS_MONO "" CACHE FILEPATH "Model weight in ONNX format for the mono modal (T1) application." FORCE)
    endif()
endif()
 
### MONO MODEL — manifest
if(EXISTS "${ONNX_MODELS_MONO_MANIFEST}")
    message(STATUS "Copying mono ONNX manifest from: ${ONNX_MODELS_MONO_MANIFEST}")
    set(ONNX_MODELS_MONO_MANIFEST_COPY_CMD
        ${CMAKE_COMMAND}
        -E
        copy
        "${ONNX_MODELS_MONO_MANIFEST}"
        "${ONNX_MODELS_DEST_DIR}/Monomodal.json")
else()
    if("${ONNX_MODELS_MONO_MANIFEST}" STREQUAL "")
        message("Mono ONNX manifest path is not specified. Application will fall back to a single T1 input for this model.")
    else()
        message(SEND_ERROR "Mono ONNX manifest path does not exist: ${ONNX_MODELS_MONO_MANIFEST}")
        set(ONNX_MODELS_MONO_MANIFEST "" CACHE FILEPATH "Manifest file for the mono modal (T1) application." FORCE)
    endif()
endif()
 
### Bi MODEL — .onnx
if(EXISTS "${ONNX_MODELS_BI}")
    message(STATUS "Copying bi ONNX model from: ${ONNX_MODELS_BI}")
    set(ONNX_MODELS_BI_ONNX_COPY_CMD
        ${CMAKE_COMMAND}
        -E
        copy
        "${ONNX_MODELS_BI}"
        "${ONNX_MODELS_DEST_DIR}/Bimodal.onnx")
else()
    if("${ONNX_MODELS_BI}" STREQUAL "")
        message("Bi ONNX model path are not specified. Application will be generate without Bi-model.")
    else()
        message(SEND_ERROR "Bi ONNX model path does not exist or is not a directory: ${ONNX_MODELS_BI}")
        set(ONNX_MODELS_BI   "" CACHE FILEPATH "Model weight in ONNX format for the bi modal (T1+Flair) application." FORCE)
    endif()
endif()
 
### Bi MODEL — manifest
if(EXISTS "${ONNX_MODELS_BI_MANIFEST}")
    message(STATUS "Copying bi ONNX manifest from: ${ONNX_MODELS_BI_MANIFEST}")
    set(ONNX_MODELS_BI_MANIFEST_COPY_CMD
        ${CMAKE_COMMAND}
        -E
        copy
        "${ONNX_MODELS_BI_MANIFEST}"
        "${ONNX_MODELS_DEST_DIR}/Bimodal.json")
else()
    if("${ONNX_MODELS_BI_MANIFEST}" STREQUAL "")
        message("Bi ONNX manifest path is not specified. Application will fall back to a single T1 input for this model.")
    else()
        message(SEND_ERROR "Bi ONNX manifest path does not exist: ${ONNX_MODELS_BI_MANIFEST}")
        set(ONNX_MODELS_BI_MANIFEST "" CACHE FILEPATH "Manifest file for the bi modal (T1+Flair) application." FORCE)
    endif()
endif()



###############################################################################
##  Atlas Downloading Block
###############################################################################
# This block creates the destination directory for the atlas files and defines
# two command variables for downloading the reference T1 image and the brain
# mask. These commands use a separate `DownloadFile.cmake` script to handle
# the actual download process.
file(MAKE_DIRECTORY ${ATLAS_DEST_DIR})

set(REF_T1_PATH "" CACHE FILEPATH "MNI reference path in nifti format (.nii.gz).")

# Temporary fix to copy nifti ref
# TODO put nifti ref on internet to download
set(Reference_T1_DOWNLOAD_CMD
    ${CMAKE_COMMAND}
    -E
    copy
    "${REF_T1_PATH}"
    "${ATLAS_DEST_DIR}/Reference_T1.nii.gz"
)

set(BRAIN_MASK_DOWNLOAD_CMD
    ${CMAKE_COMMAND}
    -DURL=${atlas_BRAIN_MASK_URL}
    -DDESTINATION=${ATLAS_DEST_DIR}/BrainMask.nrrd
    -P
    ${CMAKE_SOURCE_DIR}/cmake/DownloadFile.cmake)



###############################################################################
##  Custom Target Creation Block
###############################################################################
# This block defines the two custom targets for populating the data:
# `PopulateDataAtlas` and `PopulateDataModels`. These targets are set to run
# by default (`ALL`) and handle the execution of the download and copy commands,
# respectively, ensuring that all necessary data files are in place.
add_custom_target(PopulateDataAtlas
    ALL
    COMMENT "Populating data directories with Atlas and Mask"
    COMMAND ${Reference_T1_DOWNLOAD_CMD}
    COMMAND ${BRAIN_MASK_DOWNLOAD_CMD}
)

add_custom_target(PopulateDataModels
    ALL
    COMMENT "Populating data directories with Models"
    COMMAND ${ONNX_MODELS_MONO_ONNX_COPY_CMD}
    COMMAND ${ONNX_MODELS_MONO_MANIFEST_COPY_CMD}
    COMMAND ${ONNX_MODELS_BI_ONNX_COPY_CMD}
    COMMAND ${ONNX_MODELS_BI_MANIFEST_COPY_CMD}
)

message(STATUS "")
