# Building Lumina Player

## Requirements

- CMake ≥ 3.28
- Ninja
- GCC ≥ 13 or Clang ≥ 16 (C++20)
- Qt 6.4+ (Core, Gui, Quick — QuickControls2 QML module at runtime)
- libmpv ≥ 2.2 (client + render APIs)
- libva (optional, hardware acceleration probing)

## Fedora

```bash
sudo dnf install cmake ninja-build gcc-c++ \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtquickcontrols2-devel \
  qt6-qtsvg-devel mpv-devel libva-devel
```

> Note: do **not** install `ffmpeg-devel` from rpmfusion — it conflicts with Fedora's
> free ffmpeg libraries and is not needed (see `docs/TECHNICAL_DECISIONS.md` ADR-006).

## Build

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Binary: `build/linux-debug/lumina`

## Run

```bash
./build/linux-debug/lumina [file-to-play]
```

## Tests

```bash
ctest --preset linux-debug --output-on-failure
```

## Troubleshooting

- **Qt not found:** ensure `qt6-qtbase-devel` is installed; CMake uses `find_package(Qt6)`.
- **libmpv not found:** `pkg-config --modversion mpv` must succeed.
- **Black video / GL errors:** mpv's OpenGL render API requires the Qt scene graph to use OpenGL (forced in `main.cpp`). Check that your session supports desktop GL (not GLES-only Wayland sessions on old drivers).
- **No hardware decoding:** run with `MPV_VERBOSE=1 ./lumina` and check the log for `hwdec` init lines. VAAPI requires `libva` and a compatible GPU driver (`amdgpu` on AMD).
