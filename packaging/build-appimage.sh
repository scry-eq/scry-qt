#!/usr/bin/env bash
# Build a self-contained scry-qt-*.AppImage.
#
# Usage:
#   packaging/build-appimage.sh [BUILD_DIR]
#
# Env vars:
#   VERSION    Version string baked into the artifact filename (default: git-describe).
#   OUTPUT_DIR Where to drop the final AppImage (default: $BUILD_DIR).
#   ARCH       Target arch tag for linuxdeploy (default: x86_64).
#   TOOLS_DIR  Where to cache linuxdeploy binaries (default: $BUILD_DIR/tools).
#
# Notes:
# - For redistributable builds, run this inside an older-glibc container
#   (e.g. ubuntu:22.04). AppImages are forward-compatible with newer hosts,
#   not backward-compatible with older ones.
# - scry-qt has no pcap / capability requirements; it only opens an outbound
#   WebSocket to a scry-cpp, so no setcap or polkit dance is needed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$SRC_DIR/build}"
ARCH="${ARCH:-x86_64}"
# Normalize BUILD_DIR + OUTPUT_DIR to absolute paths — the script later cd's
# into BUILD_DIR, after which any relative path (e.g. CI's `build`) breaks
# downstream references to $LD / $APPDIR.
mkdir -p "$BUILD_DIR"
BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
TOOLS_DIR="${TOOLS_DIR:-$BUILD_DIR/tools}"
OUTPUT_DIR="${OUTPUT_DIR:-$BUILD_DIR}"
mkdir -p "$OUTPUT_DIR"
OUTPUT_DIR="$(cd "$OUTPUT_DIR" && pwd)"
APPDIR="$BUILD_DIR/AppDir"

if [[ -z "${VERSION:-}" ]]; then
  if VERSION="$(git -C "$SRC_DIR" describe --tags --dirty --always 2>/dev/null)"; then
    :
  else
    VERSION="0.0.0"
  fi
fi

if [[ ! -x "$BUILD_DIR/scry-qt" ]]; then
  echo "error: $BUILD_DIR/scry-qt not found — configure & build first:" >&2
  echo "  cmake -S '$SRC_DIR' -B '$BUILD_DIR' && cmake --build '$BUILD_DIR'" >&2
  exit 1
fi

mkdir -p "$TOOLS_DIR" "$OUTPUT_DIR"

fetch() {
  local url="$1" dst="$2"
  if [[ -x "$dst" ]]; then return 0; fi
  echo "fetching $url"
  if command -v curl >/dev/null; then
    curl -fL --retry 3 -o "$dst" "$url"
  else
    wget -O "$dst" "$url"
  fi
  chmod +x "$dst"
}

LD="$TOOLS_DIR/linuxdeploy-$ARCH.AppImage"
LDQT="$TOOLS_DIR/linuxdeploy-plugin-qt-$ARCH.AppImage"
fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage" "$LD"
fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage" "$LDQT"

# Stage a fresh AppDir so stale files from previous runs don't sneak in.
rm -rf "$APPDIR"
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# linuxdeploy-plugin-qt picks up the Qt6 install via qmake6/qtpaths6.
export QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake || true)}"

# When linuxdeploy itself is run from inside an AppImage (CI), it sometimes
# can't re-mount via FUSE. Fall back to extracting if that happens.
run_ld() {
  if ! "$LD" "$@"; then
    echo "linuxdeploy failed; retrying with --appimage-extract-and-run" >&2
    APPIMAGE_EXTRACT_AND_RUN=1 "$LD" "$@"
  fi
}

cd "$BUILD_DIR"
LDAI_OUTPUT="scry-qt-${VERSION}-${ARCH}.AppImage" run_ld \
  --appdir "$APPDIR" \
  --plugin qt \
  --output appimage \
  --desktop-file "$APPDIR/usr/share/applications/scry-qt.desktop" \
  --icon-file    "$APPDIR/usr/share/icons/hicolor/256x256/apps/scry-qt.png"

# linuxdeploy writes the .AppImage into CWD; move it to OUTPUT_DIR if different.
if [[ "$BUILD_DIR" != "$OUTPUT_DIR" ]]; then
  mv -f "$BUILD_DIR/scry-qt-${VERSION}-${ARCH}.AppImage" "$OUTPUT_DIR/"
fi

echo
echo "built: $OUTPUT_DIR/scry-qt-${VERSION}-${ARCH}.AppImage"
