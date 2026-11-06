# Contributing to Lumina Player

Thanks for helping. This page is the short version; [AGENTS.md](AGENTS.md) has the full engineering context
and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) the design.

## Before you start

- **Bug?** Search the [issues](https://github.com/mdmakky/lumina-player/issues), then open one with the
  bug template. Attach the log (Settings, **Open logs folder**).
- **Feature or big change?** Open an issue first so the design can be agreed before you write code.
- **Security problem?** Do not open a public issue. See [SECURITY.md](SECURITY.md).

## Build and test

```bash
cmake --preset linux-debug && cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure        # unit tests, fast
LUMINA_PLAYBACK_TEST=1 ./build/linux-debug/lumina tests/data/smoke.mkv   # engine self-test, exit 0 = pass
```

Run the one check that proves your change; the full UI test (`tools/run-ui-test.sh`) is for UI-wide changes.
More switches (seek benchmark, screenshots): [docs/TESTING.md](docs/TESTING.md).

## Rules of the codebase

1. The UI never contains playback logic. QML talks to `PlayerController` only; `PlayerController` talks to mpv
   only through `MpvEngine`.
2. Never block the UI thread with decoding, file scanning or other long work.
3. Never put OS-specific code in shared layers. It goes in `src/platform/`.
4. Playback must keep working when the database is unavailable.
5. Colors and spacing come from `ui/Theme/Theme.qml`, never hard-coded.
6. Do not swallow errors; log them.
7. Do not claim a feature works without building and testing it.

## Qt 6.4 compatibility (CI and Ubuntu 24.04 build against Qt 6.4)

The code must work on Qt 6.4 as well as current Qt. Known traps:

- No `font.features` in QML.
- Use `QC.Overlay` (`import QtQuick.Controls as QC`), not a bare `Overlay`, for `Overlay.modal` / `Overlay.overlay`.
- A singleton QML file must not `import Lumina.Player` unless it uses a type from it (cyclic dependency).
- Do not use JavaScript reserved words as QML identifiers (`goto` broke the compiler).
- Use `QStringLiteral` / `QLatin1String` for XML attribute lookups.

## Commits and pull requests

- Small, logical commits. Format: `type: short imperative summary` with type one of
  `feat`, `fix`, `docs`, `build`, `test`, `refactor`, `chore`, `ci`.
- Add or update a test for logic you change (`tests/unit/`). Bug fixes get a test that fails without the fix.
- Update the docs and the [changelog](CHANGELOG.md) when behaviour changes.
- CI must be green. It builds on Linux (Qt 6.4) and macOS and runs the unit tests.

## Packaging changes

RPM and DEB files are built per distro release by the `Packages` workflow, which installs each package in a
clean container and plays a test video. If you change dependencies, check the workflow result for every release
([docs/PACKAGING.md](docs/PACKAGING.md)).

## License

By contributing you agree that your work is licensed under GPL-3.0-or-later, like the rest of the project.
