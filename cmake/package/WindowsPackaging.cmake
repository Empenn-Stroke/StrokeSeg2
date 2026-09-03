# SPDX-License-Identifier: AGPL-3.0-or-later
# License details in LICENSE.txt.
#
# Author: Florent LERAY
# Date: 2025-09-05
#
# Copyright (c) 2025, INRIA


# Fichier: WindowsPackaging.cmake
#
# Description: This script contains all the Windows-specific configurations for
# the project's CPack installer, leveraging the NSIS generator. It customizes
# the installer's behavior, icons, and shortcuts to ensure a professional and
# consistent experience on the Windows platform.
#
# Main functional blocks:
# 1. Icon Configuration: Sets the icons for the installer, uninstaller, and
#    the application in the Add/Remove Programs control panel.
# 2. Installer Behavior Settings: Defines the NSIS-specific settings for
#    compression, adding the application to the system's PATH, and other
#    general installer properties.
# 3. Shortcut and Link Management: Manages the creation of desktop shortcuts
#    and adds links to the project's website and documentation within the
#    Start menu.
# 4. Post-Installation Actions: Specifies actions to be performed after the
#    installation is complete, such as automatically running the application.

# --- Icon Configuration Block ---
# This block sets the paths to the various icons used by the NSIS installer.
# It defines the icons for the taskbar, the installer wizard, the uninstaller,
# and the icon displayed in the Windows Add/Remove Programs control panel.
set(ICONS_PATH "${PROJECT_SOURCE_DIR}/resources")

set(CPACK_PACKAGE_ICON ${ICONS_PATH}/StrokeSegPWR.ico)
string(REGEX REPLACE "/" "\\\\\\\\" ICONS_PATH "${ICONS_PATH}")

set(CPACK_NSIS_MUI_ICON ${ICONS_PATH}\\\\StrokeSegPWR.ico)

set(CPACK_NSIS_MUI_UNIICON ${ICONS_PATH}\\\\StrokeSegUn.ico)

set(CPACK_NSIS_INSTALLED_ICON_NAME ${ICONS_PATH}\\\\StrokeSegPWR.ico)

# --- Installer Behavior Settings Block ---
# This block configures the core behavior of the NSIS installer. It sets the
# directory for the main executables, the uninstaller name, the compression
# algorithm (`lzma` for better compression), and enables adding the
# application's directory to the system PATH.
set(CPACK_NSIS_EXECUTABLES_DIRECTORY StrokeSeg2)

set(CPACK_NSIS_UNINSTALL_NAME "${CPACK_PACKAGE_NAME}_Uninstall")

set(CPACK_NSIS_COMPRESSOR "lzma")

set(CPACK_NSIS_MODIFY_PATH "ON")

set(CPACK_NSIS_DISPLAY_NAME ${CPACK_PACKAGE_NAME})

set(CPACK_NSIS_PACKAGE_NAME "${CPACK_PACKAGE_NAME}")

# --- Shortcut and Link Management Block ---
# This block adds custom links to the Start menu for the installed
# application. It includes links to the project's website, online
# documentation, and the local documentation, providing easy access
# to important resources for the user.
list(APPEND CPACK_NSIS_MENU_LINKS
  "https://github.com/Empenn-Stroke/StrokeSegApp" 
  "Project website") 
  
list(APPEND CPACK_NSIS_MENU_LINKS
  "https://strokeseg-doc.readthedocs.io" 
  "Online documentation") 

#list(APPEND CPACK_NSIS_MENU_LINKS
#  "doc\\\\StrokeSegBootStrapper\\\\html\\\\index.html" 
#  "Local documentation") 

# --- Post-Installation Actions Block ---
# This block configures what happens after the installer completes. It sets
# the main executable (`strokeseg2-app.exe`) to run automatically,
# and it also includes custom NSIS script commands to create and manage the
# desktop shortcut icon.
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "strokeseg2-app.exe")

set(CPACK_NSIS_MODELS_INSTALL_DIRECTORY "$COMMONPROGRAMDATA\\\\${CPACK_PACKAGE_NAME}\\\\")
set(CPACK_NSIS_ATLAS_INSTALL_DIRECTORY  "$COMMONPROGRAMDATA\\\\${CPACK_PACKAGE_NAME}\\\\")

set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)

set(CPACK_NSIS_CREATE_ICONS_EXTRA  "  StrCmp $INSTALL_DESKTOP \\\"1\\\" 0 NO_DESKTOP_SHORTCUT \\n \\\n \
  CreateShortCut \\\"$DESKTOP\\\\StrokeSeg.lnk\\\" \\\"$INSTDIR\\\\StrokeSeg2\\\\strokeseg2-app.exe\\\"  \\n \\\n \
NO_DESKTOP_SHORTCUT:")
set(CPACK_NSIS_DELETE_ICONS_EXTRA  "  Delete \\\"$DESKTOP\\\\StrokeSeg.lnk\\\"")
