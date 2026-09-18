#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <version> <path-to-OpenEPT-binary>"
    echo "Example: $0 2.0.0 build/OpenEPT"
    exit 1
fi

VERSION="$1"
GUI_BINARY="$(realpath "$2")"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

LINUXDEPLOYQT="${LINUXDEPLOYQT:-${SCRIPT_DIR}/linuxdeployqt-continuous-x86_64.AppImage}"
LINUXDEPLOYQT_URL="https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
LINUXDEPLOYQT_EXTRA_ARGS="${LINUXDEPLOYQT_EXTRA_ARGS:-}"
QMAKE="${QMAKE:-$(command -v qmake || true)}"

OUTPUT_DIR="${OUTPUT_DIR:-${SCRIPT_DIR}/output}"
WORK_DIR="${SCRIPT_DIR}/work"
APPDIR="${WORK_DIR}/AppDir"
DEBDIR="${WORK_DIR}/OpenEPT-deb"

DESKTOP_FILE="${SCRIPT_DIR}/openept.desktop"
ICON_FILE="${SCRIPT_DIR}/openept.png"

PACKAGE_NAME="openept"
ARCHITECTURE="amd64"

APPIMAGE_NAME="OpenEPT-${VERSION}-x86_64.AppImage"
DEB_NAME="${PACKAGE_NAME}_${VERSION}_${ARCHITECTURE}.deb"

info()
{
    echo
    echo "============================================================"
    echo "$1"
    echo "============================================================"
}

fail()
{
    echo "ERROR: $1" >&2
    exit 1
}

info "Checking build environment"

[ -f "${GUI_BINARY}" ] || fail "OpenEPT binary not found: ${GUI_BINARY}"
[ -n "${QMAKE}" ] && [ -x "${QMAKE}" ] || fail "qmake not found (set QMAKE=...)"
[ -f "${DESKTOP_FILE}" ] || fail "Desktop file not found: ${DESKTOP_FILE}"
[ -f "${ICON_FILE}" ] || fail "Application icon not found: ${ICON_FILE}"
command -v dpkg-deb >/dev/null 2>&1 || fail "dpkg-deb is not installed."

if [ ! -f "${LINUXDEPLOYQT}" ]; then
    info "Downloading linuxdeployqt"
    curl -fsSL -o "${LINUXDEPLOYQT}" "${LINUXDEPLOYQT_URL}"
fi
LINUXDEPLOYQT="$(realpath "${LINUXDEPLOYQT}")"
chmod +x "${LINUXDEPLOYQT}"

echo "Version:          ${VERSION}"
echo "GUI binary:       ${GUI_BINARY}"
echo "linuxdeployqt:    ${LINUXDEPLOYQT}"
echo "qmake:            ${QMAKE}"
echo "Output directory: ${OUTPUT_DIR}"

info "Preparing directories"

rm -rf "${WORK_DIR}"
mkdir -p "${OUTPUT_DIR}" "${APPDIR}/usr/bin" "${APPDIR}/usr/share/applications"
rm -f "${OUTPUT_DIR}/${APPIMAGE_NAME}" "${OUTPUT_DIR}/${DEB_NAME}"

info "Creating AppDir"

cp "${GUI_BINARY}" "${APPDIR}/usr/bin/openept"
chmod +x "${APPDIR}/usr/bin/openept"
cp "${DESKTOP_FILE}" "${APPDIR}/openept.desktop"
cp "${DESKTOP_FILE}" "${APPDIR}/usr/share/applications/openept.desktop"
cp "${ICON_FILE}" "${APPDIR}/openept.png"
ln -s usr/bin/openept "${APPDIR}/AppRun"

if [[ "${LINUXDEPLOYQT_EXTRA_ARGS}" == *-unsupported-allow-new-glibc* ]] && [ -f /usr/share/doc/libc6/copyright ]; then
    mkdir -p "${APPDIR}/usr/share/doc/libc6"
    cp /usr/share/doc/libc6/copyright "${APPDIR}/usr/share/doc/libc6/copyright"
fi

info "Building AppImage"

cd "${WORK_DIR}"

"${LINUXDEPLOYQT}" \
    "${APPDIR}/openept.desktop" \
    -qmake="${QMAKE}" \
    -appimage \
    ${LINUXDEPLOYQT_EXTRA_ARGS}

GENERATED_APPIMAGE="$(find "${WORK_DIR}" -maxdepth 1 -type f -name 'OpenEPT*.AppImage' -print -quit)"
[ -n "${GENERATED_APPIMAGE}" ] || fail "linuxdeployqt did not generate an AppImage."

mv "${GENERATED_APPIMAGE}" "${OUTPUT_DIR}/${APPIMAGE_NAME}"
chmod +x "${OUTPUT_DIR}/${APPIMAGE_NAME}"

info "Checking deployed Qt runtime"

[ -d "${APPDIR}/usr/lib" ] || fail "linuxdeployqt did not create usr/lib."
[ -d "${APPDIR}/usr/plugins" ] || fail "linuxdeployqt did not create usr/plugins."
[ -f "${APPDIR}/usr/plugins/platforms/libqxcb.so" ] || fail "Qt XCB platform plugin was not deployed."

info "Creating Debian package"

mkdir -p \
    "${DEBDIR}/DEBIAN" \
    "${DEBDIR}/usr/bin" \
    "${DEBDIR}/usr/lib/openept" \
    "${DEBDIR}/usr/share/applications" \
    "${DEBDIR}/usr/share/icons/hicolor/256x256/apps"

cp "${APPDIR}/usr/bin/openept" "${DEBDIR}/usr/lib/openept/OpenEPT"
chmod +x "${DEBDIR}/usr/lib/openept/OpenEPT"
cp -a "${APPDIR}/usr/lib" "${DEBDIR}/usr/lib/openept/"
cp -a "${APPDIR}/usr/plugins" "${DEBDIR}/usr/lib/openept/"
if [ -d "${APPDIR}/usr/translations" ]; then
    cp -a "${APPDIR}/usr/translations" "${DEBDIR}/usr/lib/openept/"
fi

cp "${DESKTOP_FILE}" "${DEBDIR}/usr/share/applications/openept.desktop"
cp "${ICON_FILE}" "${DEBDIR}/usr/share/icons/hicolor/256x256/apps/openept.png"

cat > "${DEBDIR}/usr/bin/openept" << 'LAUNCHER'
#!/bin/bash

APP_DIR="/usr/lib/openept"

export LD_LIBRARY_PATH="$APP_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$APP_DIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APP_DIR/plugins/platforms"

exec "$APP_DIR/OpenEPT" "$@"
LAUNCHER
chmod +x "${DEBDIR}/usr/bin/openept"

cat > "${DEBDIR}/DEBIAN/control" << CONTROL
Package: ${PACKAGE_NAME}
Version: ${VERSION}
Section: electronics
Priority: optional
Architecture: ${ARCHITECTURE}
Maintainer: OpenEPT Team
Homepage: https://openept.net
Description: OpenEPT Energy Profiling Tool
 OpenEPT is a graphical application for controlling the OpenEPT Energy
 Profiler, configuring the device, performing real-time measurements,
 and analyzing voltage, current, and energy consumption.
CONTROL
chmod 644 "${DEBDIR}/DEBIAN/control"

dpkg-deb --build "${DEBDIR}" "${OUTPUT_DIR}/${DEB_NAME}"

info "Validating generated packages"

[ -f "${OUTPUT_DIR}/${APPIMAGE_NAME}" ] || fail "AppImage was not generated."
[ -f "${OUTPUT_DIR}/${DEB_NAME}" ] || fail "Debian package was not generated."
dpkg-deb --info "${OUTPUT_DIR}/${DEB_NAME}" >/dev/null

rm -rf "${WORK_DIR}"

info "OpenEPT Linux deployment completed successfully"

ls -lh "${OUTPUT_DIR}/${APPIMAGE_NAME}" "${OUTPUT_DIR}/${DEB_NAME}"
