#!/usr/bin/env bash
# Cross-builds the Windows test version (MinGW-w64) from WSL Debian.
#
# One-time setup (needs sudo):   sudo apt install mingw-w64 cmake
# Build:                         bash win-debug/build_win_debug.sh
# Output:                        build-win-debug/play/
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="$ROOT/build-win-debug"
PLAY="$BUILD/play"
SDL_BIN="$ROOT/include/SDL2-devel-2.28.2-mingw/SDL2-2.28.2/x86_64-w64-mingw32/bin"

cmake -S "$ROOT" -B "$BUILD" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/win-debug/mingw-w64.cmake" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_SETUP=ON -DRAPTOR_CHEATS=ON
cmake --build "$BUILD" -j"$(nproc)"

mkdir -p "$PLAY"
cp "$BUILD/bin/raptor.exe" "$PLAY/"
cp "$SDL_BIN/SDL2.dll" "$PLAY/"
cp "$ROOT/SETUP(ADLIB).INI" "$PLAY/SETUP.INI"
cp -r "$ROOT/lang" "$PLAY/"

echo
echo "Built: $PLAY/raptor.exe"
echo "Copy FILE0000.GLB ... FILE0004.GLB from your own copy of the game into:"
echo "  $PLAY"
