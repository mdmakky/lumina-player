# Roadmap

Source of truth for milestone status (spec §18 defines the plan). Updated 2026-11-06 to match the code. Evidence column
names the test or command that proves an item; "manual" means someone watched it work; "unverified" means built but not seen.

Test evidence keys: **UT** = `ctest --preset <preset>` (11 unit test executables, see `docs/QA_REPORT.md`);
**UI** = `tools/run-ui-test.sh /tmp/long.mp4` (in-app test, `app/UiTest.cpp`); **ST** = `LUMINA_PLAYBACK_TEST=1 lumina <file>`;
**SB** = `LUMINA_SEEK_BENCH=1` (`docs/BENCHMARK.md`).

## Milestone 1 — Foundation: done
- [x] Repo, CMake + Ninja + presets, Qt bootstrap, theme (`ui/Theme/Theme.qml`), logging, CI (Linux Qt 6.4 + macOS)
- [x] Unit test harness (Qt Test via CTest). Evidence: UT, CI green.

## Milestone 2 — Playback engine: done
- [x] libmpv client + render API in Qt Quick (`MpvEngine`, `MpvVideoItem`). Evidence: ST on macOS and Fedora (Wayland + X11).
- [x] Play/pause/stop, seek (fast+exact), volume, speed, mute, track lists, error banner. Evidence: ST, UI.

## Milestone 3 — Core UX: done
- [x] Floating overlay controls with idle auto-hide, timeline with hover thumbnail, fullscreen, always-on-top, mini-player
- [x] Playlist (drag reorder, shuffle, repeat, search, M3U save/load), chapters, drag & drop, single-instance, window/volume persistence
- [x] Rebindable shortcuts with conflict detection. Evidence: UT ShortcutManager, PlaylistModel, SingleInstance; UI.
- [x] Dark/Light/OLED themes, reduced motion, Bengali translation with live switch. Evidence: UT Translations; UI.

## Milestone 4 — Subtitles & audio: done
- [x] Subtitle/audio track panel with language names, external subtitles + auto-discovery, delays, style prefs, equalizer. Evidence: UT TrackLabels; UI.
- [ ] Online subtitle download (OpenSubtitles): deliberately deferred, needs owner account.

## Milestone 5 — Performance: mostly done
- [x] Hardware decode ladder `vaapi,nvdec,auto-safe` (Linux), videotoolbox-copy (macOS). Evidence: SB on Fedora shows `hwdec=vaapi-copy`.
- [x] Seek coalescing + keyframe scrubbing; stats overlay (I). Evidence: SB (p50 ~265 ms / p95 ~430 ms exact, Fedora 1080p).
- [ ] Windows D3D11VA path: never run on Windows.
- [ ] Benchmark vs VLC/mpv on identical hardware: scripts exist (`tools/vlc_seekbench.py`, `tools/mpv_seekbench.lua`), VLC numbers missing. See `docs/PERFORMANCE_REPORT.md`.

## Milestone 6 — Advanced features: largely done
- [x] A-B loop, bookmarks, 10-band equalizer, video adjustments + presets (Default/Cinema/Vivid/Night), deinterlace, screenshots
- [x] Per-file playback profiles, advanced settings page (cache, readahead, mpv overrides). Evidence: UI; UT HistoryRepository (bookmarks, schema v3).
- [ ] Named global configuration profiles (only per-file profiles exist).

## Milestone 7 — Media library: largely done
- [x] SQLite (`library.db`), history + resume, Continue watching, favorites, folder scanner, search, hover/library thumbnails. Evidence: UT HistoryRepository, LibraryScanner, ThumbnailService; UI.
- [x] Playback works with the DB unavailable (spec rule). Evidence: code path in `ApplicationContext`; not separately tested.
- [ ] Thumbnails/library checked only on synthetic media on macOS; re-check with real movies on Linux.
- [ ] Rich metadata (tags, cover art) in library rows.

## Milestone 8 — Cross-platform: partial
- [x] macOS: bundle + dmg script (`tools/macdeploy.sh`), media keys, sleep assertion. Evidence: dmg builds; bundled-app launch not tested.
- [x] Linux: MPRIS2, screensaver inhibit, OS volume sync (pactl), Flatpak builds and runs (Wayland + X11 ST pass, VAAPI works).
- [ ] Windows: build files and experimental CI job exist, never built on Windows (`docs/WINDOWS.md`).
- [ ] RPM/DEB/AppImage: recipes in `packaging/` (unbuilt). Signing/notarization: needs owner accounts.

## Milestone 9 — Release preparation: in progress
- [x] User guide: `docs/USER_GUIDE.md`
- [x] License audit + SBOM: `THIRD_PARTY_NOTICES.md`, `tools/make-sbom.sh`, `docs/sbom.example.json`
- [x] Security review (paper review, findings open): `docs/SECURITY_REVIEW.md`
- [x] QA report and performance report drafts: `docs/QA_REPORT.md`, `docs/PERFORMANCE_REPORT.md` (owner fills TODO rows)
- [x] Package recipes (unbuilt): `packaging/rpm`, `packaging/deb`, `packaging/appimage`
- [ ] Fix high-priority security findings (S1-S3)
- [ ] Manual test matrix run on Fedora, macOS, Windows
- [ ] Build and sign real packages; Flathub screenshots; tag 0.4.0
