set(CPACK_GENERATOR "RPM")

set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/${CPACK_PACKAGE_NAME}")

string(TOLOWER "${CPACK_PACKAGE_NAME}" RPM_PKG_NAME)
set(CPACK_RPM_PACKAGE_NAME "${RPM_PKG_NAME}")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Science")
set(CPACK_RPM_PACKAGE_LICENSE "AGPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
set(CPACK_RPM_PACKAGE_RELEASE "1")

if(CPACK_PACKAGE_VENDOR)
    set(CPACK_RPM_PACKAGE_VENDOR "${CPACK_PACKAGE_VENDOR}")
else()
    set(CPACK_RPM_PACKAGE_VENDOR "Empenn Team")
endif()

#temp url, waiting for github publication
set(CPACK_RPM_PACKAGE_URL "https://github.com/Empenn-Stroke/StrokeSegApp")

set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

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

set(CPACK_RPM_SPEC_MORE_DEFINE "
%post
ln -sf ${CPACK_PACKAGING_INSTALL_PREFIX}StrokeSeg2/strokeseg2-app /usr/bin/strokeseg2-app

%postun
rm -f /usr/bin/strokeseg2-app
")

install(FILES "${LINUX_LAUNCHER_FILE}" DESTINATION "/usr/share/applications" COMPONENT BIN)

if(EXISTS "${PROJECT_SOURCE_DIR}/resources/StrokeSegPWR.png")
    install(FILES "${PROJECT_SOURCE_DIR}/resources/StrokeSegPWR.png" 
            DESTINATION "/usr/share/pixmaps" 
            RENAME "strokeseg.png"
            COMPONENT BIN)
endif()