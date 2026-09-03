# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: CMakeLists.txt
#
# Description: This is the main CMakeLists.txt for the StrokeSegApp project. It
# defines the project's properties, user options, and the overall build structure.
# It includes other CMake files to handle specific tasks like environment setup,
# external project integration, data population, and packaging.
#
# Main functional blocks:
# 1. Project Configuration: Sets the minimum required CMake version and project
#    details.
# 2. User Options: Defines configurable options that can be set by the user,
#    such as GPU support and debug mode.
# 3. URLs and Hashes: Centralizes URLs and integrity hashes for external
#    dependencies.
# 4. Path Definitions: Specifies the installation directories for all project
#    components.
# 5. Target Includes: Includes and integrates external projects, setup scripts,
#    data population scripts, and packaging logic.

# Set the minimum required CMake version
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

# Project name
project(StrokeSeg VERSION 2.0.0 LANGUAGES CXX)

# --- Compiler Options Block ---
if(MSVC)
# Enable C++ exceptions for MSVC to ensure proper error handling in the application.
    add_compile_options(/EHsc)
endif()

# --- User Options Block ---
# This block defines several options that allow the user to customize the
# build process. These options control whether to enable CUDA support for
# GPU acceleration, and whether to show debug information during CMake
# configuration.
set(ONNX_MODELS_MONO "" CACHE FILEPATH "Model weight in ONNX format for the mono modal (T1) application.")
set(ONNX_MODELS_BI   "" CACHE FILEPATH "Model weight in ONNX format for the bi modal (T1+Flair) application.")
set(ONNX_MODELS_MONO_MANIFEST "" CACHE FILEPATH "Manifest file for the mono modal (T1) application.")
set(ONNX_MODELS_BI_MANIFEST   "" CACHE FILEPATH "Manifest file for the bi modal (T1+Flair) application.")
option(STROKESEG_DEBUG_CMAKE "Show some informations like paths used during CMake configuration phase" OFF)
mark_as_advanced(STROKESEG_DEBUG_CMAKE)
set(STROKESEG_USE_GPU OFF CACHE BOOL "Enable GPU support for ONNX Runtime (specific to linux builds). Requires CUDA-compatible hardware and drivers.")

# --- URLs and Hashes Block ---
# This block centralizes all URLs and corresponding SHA256 hashes for external
# dependencies, ensuring that the project can reliably download and verify
# components like the Python interpreter, pip installer, and other libraries
# from their official sources.

if(WIN32)
    set(Anima_URL            "https://github.com/Inria-Empenn/Anima-Public/releases/download/v4.2/Anima-Windows-4.2.zip")
    set(Anima_HASH           "42ff5cca0537b9fb3cd43fe83ffb7c042926afd9e203f9a739a27ac969384ed6")

    set(dcm2niix_URL         "https://github.com/rordenlab/dcm2niix/releases/download/v1.0.20260416/dcm2niix_win.zip")
    set(dcm2niix_HASH        "969bca4fc41d5f82658acef9d0ed9cbfbd4114ec8e8668906910241fcbb2c048")
elseif (APPLE)
    set(onnxruntime_URL      "https://github.com/microsoft/onnxruntime/releases/download/v1.24.4/onnxruntime-osx-arm64-1.24.4.tgz")

    set(Anima_URL            "https://github.com/Inria-Empenn/Anima-Public/releases/download/v4.2/Anima-macOS-4.2.zip")
    set(Anima_HASH 			 "2a5c4810beb6efa8396cc31699a53ff3b26147761f22fbf1e9b9896988487b3a")

    set(dcm2niix_URL 	     "https://github.com/rordenlab/dcm2niix/releases/download/v1.0.20260416/dcm2niix_mac.zip")
    set(dcm2niix_HASH        "51e909fca34db8198d8a917cb85a00e135841d32a3c51f1154ec8d5b874de852")
else()
    set(onnxruntime_URL      "https://github.com/microsoft/onnxruntime/releases/download/v1.24.4/onnxruntime-linux-x64-gpu-1.24.4.tgz")

    set(Anima_URL            "https://github.com/Inria-Empenn/Anima-Public/releases/download/v4.2/Anima-Ubuntu-4.2.zip")
	set(Anima_HASH			 "62681DF5508DC1DBF28604FE5F6BA7CF1B10349F1E7998EC2BA1C0E496599FD4")

    set(dcm2niix_URL         "https://github.com/rordenlab/dcm2niix/releases/download/v1.0.20260416/dcm2niix_lnx.zip")
    set(dcm2niix_HASH        "e88b40f6ebbcf9f47ebfdd7bb5f0127297cb7e8b06266a91a4642b5814031bd0")
endif()


set(atlas_REF_T1_URL     "https://github.com/Inria-Empenn/Anima-Scripts-Data-Public/raw/refs/heads/master/icc_atlas/Reference_T1.nrrd")
#set(atlas_REF_T1_URL     "https://github.com/Inria-Empenn/Anima-Scripts-Data-Public/raw/refs/heads/master/icc_atlas/Reference_T1.nii.gz")
set(atlas_BRAIN_MASK_URL "https://github.com/Inria-Empenn/Anima-Scripts-Data-Public/raw/refs/heads/master/icc_atlas/BrainMask.nrrd")
set(nifti_clib_URL       "https://github.com/NIFTI-Imaging/nifti_clib.git")
set(StrokeSeg2_URL 		 "https://gitlab.inria.fr/strokeseg-pindus/v2/strokeseg2-app.git")
set(Eigen_URL			 "https://gitlab.com/libeigen/eigen.git")

set(atlas_BRAIN_MASK_HASH "b2e48e0e6e48dabe983fe8409bce96544f0d018e79f133db5c4863256d13d139")
set(atlas_REF_T1_HASH    "0c577b4f8f2c3d60abc4eb9fcaf68c33e26c9a4323b3215a9cb474a76ce4e3de")


# --- Path Definitions Block ---
# This block defines the destination directories for all components of the
# project relative to the build directory. This centralized approach
# ensures a consistent and predictable file structure for the final
# packaged application.

set(ANIMA_DEST_DIR           "${CMAKE_BINARY_DIR}/bin/Program/Anima")
set(DCM2NIIX_DEST_DIR        "${CMAKE_BINARY_DIR}/bin/Program/dcm2niix")
set(ONNX_MODELS_DEST_DIR     "${CMAKE_BINARY_DIR}/bin/AppData/Models")
set(ATLAS_DEST_DIR           "${CMAKE_BINARY_DIR}/bin/AppData/Atlas")

# --- Qt Configuration Block ---

if(WIN32)
    set(Qt6_DIR "C:/Qt/6.10.2/msvc2022_64/lib/cmake/Qt6" CACHE PATH "Path to Qt6 config")
elseif(APPLE)
    set(Qt6_DIR "/usr/local/Cellar/qt/6.10.2/lib/cmake/Qt6" CACHE PATH "Path to Qt6 config")
else()
    set(Qt6_DIR "/usr/lib/x86_64-linux-gnu/cmake/Qt6" CACHE PATH "Path to Qt6 config")
endif()

# --- Target Includes Block ---
# This block integrates the sub-projects and scripts that form the core
# of the build process. It includes the `setup_env.cmake` script for
# environment configuration, `populateData.cmake` for handling data, and
# the `packaging.cmake` script for creating the final installer.

add_subdirectory(external) # External Targets (python, pip, anima, StrokeSegLab, ...)

include(populateData.cmake)

include(cmake/package/Packaging.cmake)
