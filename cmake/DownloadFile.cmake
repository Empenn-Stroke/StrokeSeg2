# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: DownloadFile.cmake
#
# Description: This utility script is designed to download a file from a specified URL
# and save it to a given destination path. It provides optional integrity
# verification using a SHA256 hash. The script is intended to be called by other
# CMake files using the `-P` flag, with its arguments passed via `-D`.
#
# Main functional blocks:
# 1. Argument Validation: Checks if the mandatory arguments, URL and DESTINATION,
#    have been provided. If either is missing, it terminates with a fatal error.
# 2. Dynamic Command Construction: Builds the argument list for the `file(DOWNLOAD)`
#    command, including the URL and destination. It conditionally adds the
#    `EXPECTED_HASH` argument for SHA256 verification if a SHA256 value is supplied.
# 3. Download Execution and Status Check: Executes the download command and then
#    verifies the status of the operation. If the download fails, it reports a
#    fatal error with the corresponding status and log information.

# --- Arguments Validation Block ---
# This block ensures that the script receives all necessary arguments (URL and DESTINATION).
# If a required argument is missing, the script will halt the build process and
# display a clear error message to the user.
if(NOT URL OR NOT DESTINATION)
    message(FATAL_ERROR "URL and DESTINATION are mandatory arguments.")
endif()

message(STATUS "Downloading file from ${URL} to ${DESTINATION}...")

# --- Dynamic Command Construction Block ---
# This block prepares the command for file downloading. It creates a list of
# arguments for `file(DOWNLOAD)` and dynamically adds the SHA256 hash check
# if a hash value is provided. This ensures that the download is verified for
# integrity, which is crucial for external dependencies.
set(DOWNLOAD_ARGS ${URL} ${DESTINATION} STATUS status LOG log)

if(SHA256)
    message(STATUS "Using EXPECTED_HASH for SHA256 verification...")
    list(APPEND DOWNLOAD_ARGS EXPECTED_HASH SHA256=${SHA256})
endif()

# --- Execution and Status Check Block ---
# This block executes the constructed download command. After the download
# attempt, it checks the returned status. If the status is not successful,
# it displays an error with details from the log, helping to debug any
# download failures.
file(DOWNLOAD ${DOWNLOAD_ARGS})

if(NOT status)
    message(FATAL_ERROR "Download failed with status: ${status} and log: ${log}")
endif()

message(STATUS "File downloaded successfully.")

