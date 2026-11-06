#!/bin/sh
# Unbuilt recipe (syntax-checked only). Builds Lumina-x86_64.AppImage with linuxdeploy + the Qt plugin.
# Run on the OLDEST distro you want to support (AppImages need an old glibc), e.g. Ubuntu 22.04 with Qt 6 from aqtinstall.
# Needs: linuxdeploy-x86_64.AppImage and linuxdeploy-plugin-qt-x86_64.AppImage in $PATH or in ./tools-cache,
#        cmake, ninja, libmpv-dev, Qt 6 (QMAKE must point to qmake6).
# Usage: packaging/appimage/build-appimage.sh
set -e
cd "$(dirname "$0")/../.."
B=build/appimage
APPDIR=$B/AppDir
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake)}"
export QML_SOURCES_PATHS="$PWD/ui"
export LINUXDEPLOY_OUTPUT_VERSION="$(sed -n 's/^project(Lumina VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt)"
PATH="$PWD/tools-cache:$PATH"

cmake -S . -B $B -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build $B
DESTDIR="$PWD/$APPDIR" cmake --install $B

# libmpv/FFmpeg are bundled from the build host; GPU drivers (libva, mesa) must NOT be bundled.
linuxdeploy-x86_64.AppImage --appdir "$APPDIR" --plugin qt \
    --desktop-file "$APPDIR/usr/share/applications/io.github.mdmakky.Lumina.desktop" \
    --icon-file "$APPDIR/usr/share/icons/hicolor/256x256/apps/io.github.mdmakky.Lumina.png" \
    --exclude-library 'libva*' --exclude-library 'libdrm*' --exclude-library 'libGL*' --exclude-library 'libEGL*' \
    --output appimage
echo "Done: $(ls Lumina*.AppImage lumina*.AppImage 2>/dev/null)"
