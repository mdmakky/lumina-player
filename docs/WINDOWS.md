# Windows

**Status: experimental. CI builds `lumina.exe` and 15 of 16 unit tests pass on Windows (SingleInstance fails, cause unknown); the author has NEVER run the app on a Windows machine** (developed on macOS/Linux). Everything here, including the CI job, is unverified; expect fixes.

## What exists
- `CMakeLists.txt`: libmpv via pkg-config, else `-DLIBMPV_ROOT=<mpv-dev>` (needs `include/mpv/client.h` and `lib/mpv.lib` or `libmpv.dll.a`); `WIN32_EXECUTABLE`; version resource + icon (`packaging/windows/lumina.rc.in`, `resources/icons/app/lumina.ico`); install rule for the libmpv DLL.
- `src/platform/windows/SleepInhibitor_win.*`: `SetThreadExecutionState` while video plays (wired in `PlayerController` under `Q_OS_WIN`).
- CI job `build-windows` (MSYS2 MinGW64, `continue-on-error`), uploads `lumina.exe` + windeployqt output.

## Build with MSYS2 (easiest)
In a "MSYS2 MINGW64" shell:
```bash
pacman -S --needed mingw-w64-x86_64-{toolchain,cmake,ninja,pkgconf,qt6-base,qt6-declarative,qt6-svg,qt6-tools,mpv}
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build && ctest --test-dir build
```
Package names are from memory; check with `pacman -Ss qt6`.

## Build with MSVC
1. Qt 6.4+ MSVC kit from the Qt installer (Qt Quick, Qt Shader Tools; the SQLite SQL driver is bundled).
2. libmpv dev package: `mpv-dev-x86_64-*.7z` from the shinchiro builds (https://sourceforge.net/projects/mpv-player-windows/files/libmpv/). It contains `libmpv-2.dll`, `libmpv.dll.a`, `include/`. MSVC needs an import lib: generate with `gendef libmpv-2.dll` then `lib /def:libmpv-2.def /machine:x64 /out:mpv.lib`, and put `mpv.lib` under `<root>/lib` or `<root>`.
3. From a VS x64 prompt:
```bat
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\6.x\msvc2022_64 -DLIBMPV_ROOT=C:\mpv-dev
cmake --build build
```
vcpkg's `mpv` port may not expose the libmpv client API; prefer the SDK.

## Runtime
Run `windeployqt6 --qmldir ui build\bin\lumina.exe` (or `windeployqt`), then put `libmpv-2.dll` (for MinGW builds also its dependent DLLs; `ldd lumina.exe` lists them) beside `lumina.exe`. `cmake --install` copies the mpv DLL if found.

## Known gaps
- Never run: the app forces Qt's desktop OpenGL scene graph for mpv's render API; ANGLE/driver behaviour untested.
- hwdec uses mpv `auto-safe` (D3D11VA expected); no Windows tuning.
- No installer (NSIS/WiX), code signing, file associations, or media-key/taskbar integration.
- stderr logging is invisible in a GUI-subsystem exe; no log file yet.
- `LUMINA_PLAYBACK_TEST` / UI tests use `/tmp` paths and are not Windows-ready; CI runs unit tests only.
- Single-instance uses `QLocalServer` (named pipe); untested on Windows.
