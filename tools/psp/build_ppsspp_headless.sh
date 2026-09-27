#!/usr/bin/env bash
set -euo pipefail
SRC="${PPSSPP_SRC:-$(cd "$(dirname "$0")/../../.." && pwd)/godot4-psp-tools/ppsspp-1.20.4}"
DEPS="$HOME/ppsspp-deps"
BUILD="$HOME/ppsspp-build"
mkdir -p "$DEPS" "$BUILD"
if [ ! -f "$DEPS/inst/lib/cmake/SDL2/SDL2Config.cmake" ]; then
  cd "$DEPS"
  [ -d SDL2-2.30.9 ] || curl -sL https://github.com/libsdl-org/SDL/releases/download/release-2.30.9/SDL2-2.30.9.tar.gz | tar xz
  cmake -G Ninja -S SDL2-2.30.9 -B sdlb -DCMAKE_INSTALL_PREFIX="$DEPS/inst" -DCMAKE_BUILD_TYPE=Release -DSDL_TEST=OFF
  ninja -C sdlb install
fi
cd "$BUILD"
cmake -G Ninja "$SRC" -DHEADLESS=ON -DCMAKE_BUILD_TYPE=Release -DUSING_QT_UI=OFF -DUNITTEST=OFF \
  -DUSE_DISCORD=OFF -DUSE_FFMPEG=OFF -DCMAKE_PREFIX_PATH="$DEPS/inst"
ninja PPSSPPHeadless
ls -la "$BUILD/PPSSPPHeadless"
