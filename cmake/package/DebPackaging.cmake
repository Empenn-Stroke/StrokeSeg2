# --- Linux / Debian Specific Packaging Configuration ---

set(CPACK_GENERATOR "DEB")

set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}")

set(CPACK_DEB_COMPONENT_INSTALL OFF)

string(TOLOWER "${CPACK_PACKAGE_NAME}" DEBIAN_PKG_NAME)
set(CPACK_DEBIAN_PACKAGE_NAME "${DEBIAN_PKG_NAME}")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")

if(CPACK_PACKAGE_CONTACT)
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
else()
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Empenn Team <${CPACK_PACKAGE_NAME}@inria.fr>")
endif()

set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Empenn-Stroke/StrokeSegApp")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS_PRIVATE_DIRS "${CMAKE_BINARY_DIR}/ExtProjs/onnxruntime/src/onnxruntime/lib")

set(LINUX_LAUNCHER_FILE "${CMAKE_BINARY_DIR}/strokeseg.desktop")
file(WRITE "${LINUX_LAUNCHER_FILE}"
    "[Desktop Entry]\n"
    "Version=1.0\n"
    "Type=Application\n"
    "Name=${CPACK_PACKAGE_NAME}\n"
    "Comment=${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\n"
    "Exec=${CPACK_PACKAGING_INSTALL_PREFIX}/StrokeSeg2/strokeseg2-app\n"
    "Icon=strokeseg\n"
    "Categories=Science;MedicalSoftware;\n"
    "Terminal=false\n"
)

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "")

file(WRITE "${CMAKE_BINARY_DIR}/postinst" "
#!/bin/sh
set -e
ln -sf ${CPACK_PACKAGING_INSTALL_PREFIX}/StrokeSeg2/strokeseg2-app /usr/bin/strokeseg2-app
exit 0
")

file(WRITE "${CMAKE_BINARY_DIR}/postrm" "
#!/bin/sh
set -e
rm -f /usr/bin/strokeseg2-app
exit 0
")

file(CHMOD "${CMAKE_BINARY_DIR}/postinst" 
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
file(CHMOD "${CMAKE_BINARY_DIR}/postrm" 
     PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

list(APPEND CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_BINARY_DIR}/postinst" "${CMAKE_BINARY_DIR}/postrm")

install(FILES "${LINUX_LAUNCHER_FILE}" DESTINATION "/usr/share/applications" COMPONENT BIN)

if(EXISTS "${PROJECT_SOURCE_DIR}/resources/StrokeSegPWR.png")
    install(FILES "${PROJECT_SOURCE_DIR}/resources/StrokeSegPWR.png" 
            DESTINATION "/usr/share/pixmaps" 
            RENAME "strokeseg.png"
            COMPONENT BIN)
endif()