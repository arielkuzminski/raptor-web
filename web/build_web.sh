#!/usr/bin/env bash
# Buduje Raptora do WebAssembly (Emscripten).
# Uruchamiać w WSL Debian:  bash /mnt/r/repo/rea/raptor-src/web/build_web.sh
set -euo pipefail

SRC="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$SRC/web/dist"
OBJ="$HOME/raptor-build/obj"          # obiekty na dysku Linuksa - szybciej niż /mnt/r
# Lokalnie: emsdk w ~/tools/emsdk. W CI (setup-emsdk) emcc jest już w PATH.
if ! command -v emcc >/dev/null 2>&1; then
    source "$HOME/tools/emsdk/emsdk_env.sh" >/dev/null 2>&1
fi

mkdir -p "$OUT" "$OBJ"

CFLAGS=(-O2 -sUSE_SDL=2
        -I"$SRC/include/textscreen/include" -I"$SRC/include/TinySoundFont" -I"$SRC/src"
        -Wno-everything)

# Lista plików jak w CMakeLists.txt (bez SETUP - w przeglądarce go nie ma)
GAME_SRC=(tile gssapi objects tile_a windows flame i_lastscr joyapi kbdapi glbapi
          i_video musapi movie_a ptrapi_a swdapi imsapi store i_oplmusic bonus anims
          shadow movie m_misc shots opl3 eshot gfxapi_a dspapi vmemapi rap demo prefapi
          gfxapi intro help input fx ptrapi loadsave enemy mputsf lang interp pseudo3d)
TXT_SRC=(txt_conditional txt_checkbox txt_desktop txt_dropdown txt_fileselect txt_gui
         txt_inputbox txt_io txt_button txt_label txt_radiobutton txt_scrollpane
         txt_separator txt_spinctrl txt_sdl txt_strut txt_table txt_utf8 txt_widget
         txt_window txt_window_action)

compile() {  # $1 = plik źródłowy, $2 = plik .o
    if [ ! -f "$2" ] || [ "$1" -nt "$2" ] || [ -n "$(find "$SRC/src" -name '*.h' -newer "$2" -print -quit)" ]; then
        echo "  emcc $(basename "$1")"
        emcc "${CFLAGS[@]}" -c "$1" -o "$2"
    fi
}
JOBS=$(nproc)
throttle() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n; done; }

echo "== Kompilacja ($JOBS równolegle)"
OBJS=()
for f in "${GAME_SRC[@]}"; do throttle; compile "$SRC/src/$f.cpp" "$OBJ/$f.o" & OBJS+=("$OBJ/$f.o"); done
for f in "${TXT_SRC[@]}";  do throttle; compile "$SRC/include/textscreen/include/$f.c" "$OBJ/$f.o" & OBJS+=("$OBJ/$f.o"); done
for pid in $(jobs -rp); do wait "$pid"; done

echo "== Linkowanie"
em++ -O2 "${OBJS[@]}" -o "$OUT/raptor.js" \
    -sUSE_SDL=2 \
    -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 \
    -sALLOW_MEMORY_GROWTH -sINITIAL_MEMORY=67108864 -sSTACK_SIZE=1048576 \
    -sINVOKE_RUN=0 -sEXIT_RUNTIME=0 -sFORCE_FILESYSTEM \
    -sEXPORTED_RUNTIME_METHODS=FS,callMain,addRunDependency,removeRunDependency \
    -lidbfs.js \
    --embed-file "$SRC/lang@/lang"

cp "$SRC/web/index.html" "$OUT/index.html"
ls -lh "$OUT"
