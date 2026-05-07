#!/usr/bin/env bash
# ==================================================================
# Grid Simulator -- Linux portable build
# Produces dist_linux/ containing a self-contained binary + libs.
# If linuxdeployqt is available it also produces an AppImage.
# ==================================================================





# MIT License

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.



set -euo pipefail

SCRIPTDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
APPNAME="GridSimUserControl"
BUILDDIR="$SCRIPTDIR/build_release"
DEPLOYDIR="$SCRIPTDIR/dist_linux"

# Colour helpers
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'
ok()   { echo -e "${GREEN}✓${NC} $*"; }
warn() { echo -e "${YELLOW}⚠${NC} $*"; }
err()  { echo -e "${RED}✗ ERROR:${NC} $*"; exit 1; }
info() { echo -e "${CYAN}  $*${NC}"; }
step() { echo -e "\n${BOLD}$*${NC}"; }

echo -e "${BOLD}=================================================="
echo "  Grid Simulator — Linux portable build"
echo -e "==================================================${NC}"

# ------------------------------------------------------------------
# 1. Find qmake
# ------------------------------------------------------------------
step "Locating qmake..."
QMAKE=""
for candidate in qmake6 qmake-qt6 qmake qmake-qt5 qmake5; do
    if command -v "$candidate" &>/dev/null; then
        QMAKE=$(command -v "$candidate")
        break
    fi
done

if [ -z "$QMAKE" ]; then
    err "qmake not found.\nInstall Qt dev packages:\n  sudo apt install qt5-qmake qtbase5-dev libqt5serialport5-dev"
fi

QT_VERSION=$($QMAKE -query QT_VERSION 2>/dev/null || echo "unknown")
QT_INSTALL_LIBS=$($QMAKE -query QT_INSTALL_LIBS)
QT_INSTALL_PLUGINS=$($QMAKE -query QT_INSTALL_PLUGINS)
ok "qmake: $QMAKE (Qt $QT_VERSION)"

# ------------------------------------------------------------------
# 2. Check build tools
# ------------------------------------------------------------------
command -v make &>/dev/null  || err "make not found.  sudo apt install build-essential"
command -v g++ &>/dev/null   || err "g++ not found.   sudo apt install build-essential"
command -v strip &>/dev/null || warn "strip not found — binary won't be stripped"
ok "Build tools found"

# ------------------------------------------------------------------
# 3. Build
# ------------------------------------------------------------------
step "Building..."
rm -rf "$BUILDDIR"
mkdir -p "$BUILDDIR"

cd "$BUILDDIR"
echo "Running qmake..."
$QMAKE "$SCRIPTDIR/${APPNAME}.pro" CONFIG+=release 2>&1 \
    || err "qmake failed. Check .pro file and Qt installation."

CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)
echo "Running make -j$CORES..."
make -j"$CORES" 2>&1 || err "make failed. See compiler errors above."
cd "$SCRIPTDIR"

# Find binary
BINARY=$(find "$BUILDDIR" -maxdepth 3 -name "$APPNAME" -type f -executable 2>/dev/null | head -1)
[ -z "$BINARY" ] && err "Built binary not found in $BUILDDIR"
ok "Binary: $BINARY"

# Strip
if command -v strip &>/dev/null; then
    strip "$BINARY"
    BINSIZE=$(du -sh "$BINARY" | cut -f1)
    ok "Stripped — size: $BINSIZE"
fi

# ------------------------------------------------------------------
# 4. Create deploy directory
# ------------------------------------------------------------------
step "Assembling portable directory: $DEPLOYDIR/"
rm -rf "$DEPLOYDIR"
mkdir -p "$DEPLOYDIR/lib"
mkdir -p "$DEPLOYDIR/plugins/platforms"
mkdir -p "$DEPLOYDIR/plugins/serialport"

cp "$BINARY" "$DEPLOYDIR/$APPNAME"

# Bundle shared libraries using ldd
echo "Collecting shared libraries..."
BUNDLED=0
while IFS= read -r line; do
    # ldd output: "  libFoo.so => /path/to/libFoo.so (0xADDR)"
    lib=$(echo "$line" | awk '{print $3}')
    [[ "$lib" =~ ^/ ]] || continue
    [ -f "$lib" ] || continue
    libname=$(basename "$lib")

    # Bundle Qt libs, libserial, libGL, and related.
    # Skip core system libs (glibc, libpthread, libstdc++ etc.)
    if echo "$libname" | grep -qE \
        "^(libQt|libserial|libGL|libEGL|libxcb|libxkb|libicui18m|libicudata|libicuuc)"; then
        if cp -n "$lib" "$DEPLOYDIR/lib/" 2>/dev/null; then
            (( BUNDLED++ )) || true
        fi
    fi
done < <(ldd "$BINARY" 2>/dev/null)
ok "Bundled $BUNDLED libraries"

# Qt platform plugin — required for any Qt GUI app on Linux
if [ -f "$QT_INSTALL_PLUGINS/platforms/libqxcb.so" ]; then
    cp "$QT_INSTALL_PLUGINS/platforms/libqxcb.so" \
       "$DEPLOYDIR/plugins/platforms/"
    ok "Platform plugin: libqxcb.so"

    # libqxcb itself depends on xcb libs — collect them too
    while IFS= read -r line; do
        lib=$(echo "$line" | awk '{print $3}')
        [[ "$lib" =~ ^/ ]] || continue
        [ -f "$lib" ] || continue
        libname=$(basename "$lib")
        if echo "$libname" | grep -qE "^(libxcb|libX|libGL|libEGL)"; then
            cp -n "$lib" "$DEPLOYDIR/lib/" 2>/dev/null || true
        fi
    done < <(ldd "$QT_INSTALL_PLUGINS/platforms/libqxcb.so" 2>/dev/null)
fi

# Minimal fallback platform (for headless / testing)
[ -f "$QT_INSTALL_PLUGINS/platforms/libqminimal.so" ] && \
    cp "$QT_INSTALL_PLUGINS/platforms/libqminimal.so" \
       "$DEPLOYDIR/plugins/platforms/" 2>/dev/null || true

# xcb-gl integration plugin (needed on some distros for rendering)
if [ -d "$QT_INSTALL_PLUGINS/xcbglintegrations" ]; then
    mkdir -p "$DEPLOYDIR/plugins/xcbglintegrations"
    cp "$QT_INSTALL_PLUGINS/xcbglintegrations"/*.so \
       "$DEPLOYDIR/plugins/xcbglintegrations/" 2>/dev/null || true
fi

# Qt SerialPort plugin
if [ -d "$QT_INSTALL_PLUGINS/serialport" ]; then
    cp "$QT_INSTALL_PLUGINS/serialport"/*.so \
       "$DEPLOYDIR/plugins/serialport/" 2>/dev/null || true
    ok "SerialPort plugins bundled"
fi

# qt.conf tells Qt where to find its plugins relative to the binary
cat > "$DEPLOYDIR/qt.conf" << 'EOF'
[Paths]
Plugins = plugins
EOF

# ------------------------------------------------------------------
# 5. Write the wrapper launcher (sets LD_LIBRARY_PATH)
# ------------------------------------------------------------------
cat > "$DEPLOYDIR/launch.sh" << 'LAUNCHER'
#!/usr/bin/env bash
# Auto-generated launcher — sets library paths relative to this directory.
APPDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export LD_LIBRARY_PATH="$APPDIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="$APPDIR/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="$APPDIR/plugins/platforms"
exec "$APPDIR/GridSimUserControl" "$@"
LAUNCHER
chmod +x "$DEPLOYDIR/launch.sh"

ok "Wrapper launcher written: $DEPLOYDIR/launch.sh"

# ------------------------------------------------------------------
# 6. Try linuxdeployqt / AppImage (optional, requires the tool)
# ------------------------------------------------------------------
step "Checking for linuxdeployqt..."
LDQT=""
for candidate in linuxdeployqt \
                 ./linuxdeployqt \
                 ./linuxdeployqt-continuous-x86_64.AppImage; do
    command -v "$candidate" &>/dev/null && { LDQT="$candidate"; break; }
    [ -x "$candidate" ] && { LDQT="$candidate"; break; }
done

if [ -n "$LDQT" ]; then
    ok "Found: $LDQT"
    echo "Building AppImage..."

    # Create a temporary AppDir structure that linuxdeployqt expects
    APPDIR_TMP="$(mktemp -d)"
    mkdir -p "$APPDIR_TMP/usr/bin"
    mkdir -p "$APPDIR_TMP/usr/share/applications"
    mkdir -p "$APPDIR_TMP/usr/share/icons/hicolor/256x256/apps"
    cp "$BINARY" "$APPDIR_TMP/usr/bin/"

    cat > "$APPDIR_TMP/usr/share/applications/gridsim.desktop" << 'DESKTOP'
[Desktop Entry]
Name=Grid Simulator
Comment=Power grid simulation control panel
Exec=GridSimUserControl
Icon=gridsim
Terminal=false
Type=Application
Categories=Science;Education;
DESKTOP

    # Create a minimal placeholder icon (1x1 px PNG) if no real icon exists
    if [ ! -f "$SCRIPTDIR/gridsim.png" ]; then
        # 1x1 transparent PNG, base64 encoded
        echo "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==" \
            | base64 -d > "$APPDIR_TMP/usr/share/icons/hicolor/256x256/apps/gridsim.png"
    else
        cp "$SCRIPTDIR/gridsim.png" \
           "$APPDIR_TMP/usr/share/icons/hicolor/256x256/apps/"
    fi

    "$LDQT" "$APPDIR_TMP/usr/bin/$APPNAME" \
        -qmake="$QMAKE" \
        -bundle-non-qt-libs \
        -appimage \
        -desktop-file="$APPDIR_TMP/usr/share/applications/gridsim.desktop" \
        2>&1 && ok "AppImage created in $SCRIPTDIR" \
              || warn "AppImage creation failed — directory bundle is still usable"

    rm -rf "$APPDIR_TMP"
else
    warn "linuxdeployqt not found — AppImage not created"
    info "Download from: https://github.com/probonopd/linuxdeployqt/releases"
    info "  wget -O linuxdeployqt \\"
    info "    https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
    info "  chmod +x linuxdeployqt"
    info "Then re-run this script."
fi

# ------------------------------------------------------------------
# Done
# ------------------------------------------------------------------
echo -e "\n${BOLD}=================================================="
echo "  SUCCESS"
echo -e "  Portable directory: $DEPLOYDIR/"
echo "  Run with:           $DEPLOYDIR/launch.sh"
echo -e "==================================================${NC}"
