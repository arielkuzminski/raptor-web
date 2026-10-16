# Windows debug build

Cross-compiles Raptor for Windows (64-bit) from WSL Debian with MinGW-w64.
Use it to test on Windows. It is not the production build (that one is the web build on the `web` branch).

## Setup (once)

```bash
sudo apt install mingw-w64 cmake
```

## Build

From WSL, in the repository root:

```bash
bash win-debug/build_win_debug.sh
```

The output goes to `build-win-debug/play/` (ignored by git):

- `raptor.exe`, `SDL2.dll`
- `SETUP.INI`, `lang/`

## Game data

Copy `FILE0000.GLB` … `FILE0004.GLB` from your own copy of the game into `build-win-debug/play/`.
Never commit these files: they are not ours to distribute.

## Run

In PowerShell:

```powershell
cd build-win-debug\play
.\raptor.exe
```

Debug cheat (god mode): set `S_HOST=CASTLE` in the same window before starting the game.

```powershell
$env:S_HOST="CASTLE"; .\raptor.exe
```
