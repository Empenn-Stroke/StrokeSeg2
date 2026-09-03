# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA

# --- `ExternalProject_Add` Call Block ---
# This block defines the external project configuration for Anima. It sets
# all the necessary paths and tells CMake how to handle the project.

set(NU_PACKAGE_NAME "Microsoft.Windows.CppWinRT")
set(NU_PACKAGE_VERSION "2.0.250303.1")
set(NU_PACKAGE_DEST "${CMAKE_BINARY_DIR}/nuget_packages")

add_nuget_package(${NU_PACKAGE_NAME} ${NU_PACKAGE_VERSION} ${NU_PACKAGE_DEST})
