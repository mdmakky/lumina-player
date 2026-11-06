# QA Report (draft for 0.4.0, 2026-11-06)

Honest summary: automated coverage is good for logic and for the UI on macOS; real-hardware evidence exists only for one Fedora
laptop (AMD APU, Wayland). Windows is untested. Boxes below are for the owner to tick during release testing.

## What was tested where

| Check | Command | macOS | Fedora 45 | Windows | CI |
|---|---|---|---|---|---|
| Unit tests, 11 CTest executables | `ctest --preset <preset>` | 11/11 | 11/11 (after the volume/popup changes) | not run | Linux Qt 6.4 + macOS |
| In-app UI test, about 180 checks | `tools/run-ui-test.sh /tmp/long.mp4` (needs a >= 40 s file) | pass (before popups became modal) | not run | not run | not in CI |
| Playback self-test (load, play, exact seek, pause, resume) | `LUMINA_PLAYBACK_TEST=1 lumina <file>` | pass | pass (native, Flatpak Wayland + X11) | not run | no |
| Seek benchmark | `LUMINA_SEEK_BENCH=1` (docs/BENCHMARK.md) | done | done (1 file) | not run | no |
| Flatpak build, `desktop-file-validate`, `appstreamcli validate --no-net` | see docs/PACKAGING.md | n/a | pass | n/a | no |
| dmg build | `tools/macdeploy.sh` | builds (83 MB) | n/a | n/a | no |

Unit test executables (`tests/unit/`, about 50 test functions in total, counted from `void test...` slots): TimeUtils, UrlUtils, TrackLabels,
PlaylistModel, ShortcutManager, SingleInstance, Translations, HistoryRepository, LibraryScanner, Mpris, ThumbnailService.
The UI test (`app/UiTest.cpp`, switch `LUMINA_UI_TEST`) covers settings navigation/search/reset, shortcut rebinding, equalizer,
bookmarks, themes, language switch, hover thumbnail, Open URL with a local HTTP server, keyboard navigation of popups.

## Known gaps
- UI test not re-run after popups became modal (HANDOFF item); run it once on macOS and Fedora before tagging.
- Windows: never built or run. D3D11VA unverified.
- macOS bundle: app launch/playback from the `.app`, Finder "Open With", signing/notarization untested.
- Flatpak: sound by ear, MPRIS, portals/file dialogs, Flathub lint untested.
- Fedora: Bengali build (needs `qt6-linguist qt6-qttools-devel`), new popup panels, mouse-wheel volume sensitivity, Always-on-top on Wayland.
- Library, thumbnails and seek preview verified with synthetic media only.
- No fuzzing or sanitizer run (see `docs/SECURITY_REVIEW.md`); security findings S1-S3 open.
- Wide hardware coverage (NVIDIA NVDEC, Intel VAAPI, HDR, multi-monitor, HiDPI scaling) not tested.

## Manual test matrix
Tick per platform: F = Fedora (Flatpak or native), M = macOS, W = Windows.

| Test | F | M | W |
|---|---|---|---|
| Install from package, first launch, icon and name correct | [ ] | [ ] | [ ] |
| Open H.264 mkv via Ctrl+O, drag & drop, CLI argument, "Open With" | [ ] | [ ] | [ ] |
| Play / pause / seek keys, timeline click, drag scrub, no freeze | [ ] | [ ] | [ ] |
| Audio works for AAC, E-AC3, DTS; banner shows fix when missing | [ ] | [ ] | [ ] |
| Hardware decode active (stats `I` shows vaapi / videotoolbox / d3d11va) | [ ] | [ ] | [ ] |
| Volume slider, Up/Down, wheel, mute; OS volume sync on Linux | [ ] | [ ] | [ ] |
| Subtitles: embedded, external file, delay G/H, style settings | [ ] | [ ] | [ ] |
| Audio tracks, speed X/C/Z, A-B loop, bookmarks, chapters | [ ] | [ ] | [ ] |
| Playlist: add, reorder, shuffle, repeat, M3U save/load, auto-advance | [ ] | [ ] | [ ] |
| Resume position after restart; Continue watching list | [ ] | [ ] | [ ] |
| Library: add folder, scan, search, favorites; works with DB deleted | [ ] | [ ] | [ ] |
| Open URL: direct mp4, HLS, page URL with yt-dlp installed/absent | [ ] | [ ] | [ ] |
| Fullscreen, always on top, mini-player, themes, reduced motion | [ ] | [ ] | [ ] |
| Settings: every page opens, search, reset, rebind shortcut + conflict | [ ] | [ ] | [ ] |
| Bengali language switch (live), no clipped labels | [ ] | [ ] | [ ] |
| Single instance: second launch forwards file | [ ] | [ ] | [ ] |
| MPRIS widget / media keys / screensaver or sleep inhibited during playback | [ ] | [ ] | [ ] |
| Corrupt file shows error banner, app stays alive | [ ] | [ ] | [ ] |
| 4K60 and 10-bit HEVC play without drops | [ ] | [ ] | [ ] |
| 2 h movie playback, no memory growth (Activity Monitor / `top`) | [ ] | [ ] | [ ] |
| Uninstall leaves no stray files except user data | [ ] | [ ] | [ ] |
