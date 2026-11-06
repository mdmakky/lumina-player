# Handoff — state at the end of the macOS development session (2026-11-05)

Read `AGENTS.md` first (architecture, rules, test switches). This file is the *session memory*: what happened, what was
learned on the owner's Fedora machine, what is still open, and how the owner likes to work.

## Who / how to work with the owner
- Owner: `mdmakky` (GitHub). Writes **Bengali in Latin letters mixed with English** ("kaj korche", "sound ashe").
  Reply in the same style (short, direct). Primary target is **Linux (Fedora 45, GNOME/Wayland, AMD Ryzen APU)**.
- Wants: fast progress, fewer words, **do not re-run the full test suite repeatedly** ("ek test bar bar kore somoy nosto
  koris na"). Run the one check that proves the fix, then move on.
- Wants a professional, polished UI (they judged early designs harshly); screenshots they send are the truth.
- Their terminal is sometimes inside a sandboxed IDE (ZCode) where `sudo` is blocked ("no new privileges"): sudo commands
  must be run from a normal terminal. They also sometimes paste placeholder paths literally — give real paths.
- Their pain that started the project: VLC freezes/drops frames on seek on Fedora. Smooth seeking is the core promise.

## Repo state (all pushed to `mdmakky/lumina-player`, main; version 0.3.0)
Done and tested on macOS (unit tests 11/11, in-app UI test ~180 checks, CI green on Linux Qt 6.4 + macOS):
playback engine (mpv render API), floating-overlay UI, playlist (drag reorder/shuffle/repeat/M3U), chapters, A-B loop,
bookmarks, equalizer, audio/subtitle track panel (+delays), screenshots, mini-player, Dark/Light/OLED themes, reduced
motion, Settings (10 pages + search + reset), rebindable shortcuts, SQLite history + Continue watching + favorites,
optional media library, per-file settings, network streams (Open URL, yt-dlp), seek-bar hover thumbnails, seek coalescing,
Bengali translation (live language switch), MPRIS + screensaver inhibit (Linux), packaging files.

Test switches: see AGENTS.md ("Self-test env switches"). On macOS the binary is `build/mac/bin/lumina`; on Linux it is
`build/linux-debug/lumina`. UI test: `tools/run-ui-test.sh /tmp/long.mp4` (forces focus; needs a >=40 s file).

## What the owner's Fedora machine showed (real hardware — the only Linux evidence we have)
- App builds and runs (Qt 6.11.2, libmpv 2.5/mpv 0.41, Wayland). Video, subtitles, resume, MPRIS registration and screensaver
  inhibit work. **Sound works** after installing RPM Fusion `libavcodec-freeworld` (movie audio was E-AC3; Fedora's
  `ffmpeg-free` lacks E-AC3/DTS). The app now shows a "No sound ... run: sudo dnf install libavcodec-freeworld" banner.
- VAAPI: first run fell back to software H.264 (`No support for codec h264 profile 100`) because only
  `libva-intel-media-driver` was installed. After `mesa-va-drivers-freeworld` `vainfo` lists H264 High VLD on radeonsi.
  **Not yet confirmed**: that Lumina's stats overlay (`I`) now shows `vaapi`.
- Not yet reported by the owner: seek smoothness vs VLC (use `LUMINA_SEEK_BENCH=1`, docs/BENCHMARK.md), MPRIS widget,
  Bengali (they have not installed `qt6-linguist qt6-qttools-devel`, so the build has no translations), panel redesign on Linux.
- Helper scripts: `tools/diagnose-linux.sh <movie>` (asks for real movie path), `tools/install-deps-fedora.sh`.

## Decisions and gotchas learned (do not re-learn them)
- macOS only: ao `coreaudio,avfoundation` (avfoundation alone delays speed changes ~4 s; coreaudio alone cannot open 48 kHz
  devices), `audio-channels=stereo,5.1,7.1`, GL 3.3 core profile, `hwdec=videotoolbox-copy` (screenshots need it). All guarded
  by `Q_OS_MACOS`. Linux uses mpv defaults + `hwdec=vaapi,nvdec,auto-safe`.
- Video orientation: `flip_y=0` (screenshots on macOS showed upside-down text with 1). **Unverified on Linux** — the owner's
  movie looked upright on Fedora, so it is probably right. `LUMINA_FLIP_Y=1` flips it if a platform differs.
- CI uses Qt 6.4: no `font.features`, no JS reserved words as identifiers (`goto` broke the QML compiler), use
  `QStringLiteral`/`QLatin1String` in XML attribute lookups. Windows CI job is manual-only (MSYS2 package names unverified).
- Self-tests/UI tests use isolated QSettings/DB (`Lumina Player UITest`); they once polluted the real settings.
- Flatpak manifest (`packaging/flatpak`) builds FFmpeg from source (all decoders) but has **never been built**.
- Not done on purpose (needs the owner's accounts): OpenSubtitles download, code signing/notarization, auto-update.

## Linux session (2026-11-06) — what was done on Fedora, and what the macOS agent must check
Owner tested by hand on Fedora (GNOME/Wayland); only the items marked "tested" were seen working by the owner.

**Verified on Fedora**
- Seek benchmark, Fight Club 1080p H.264 mkv: hwdec `vaapi-copy` works; exact seek p50 ~265 ms / p95 ~430 ms, keyframe seek p50 ~35 ms,
  0 dropped frames, spam settles ~1.7 s. **VLC comparison not done** (needs `sudo dnf install vlc python3-vlc` from a normal terminal, then `tools/vlc_seekbench.py`).
- **Flatpak now builds** and runs (Wayland and X11 self-test pass, VAAPI works inside the sandbox, E-AC3/DTS decoders compiled in). Manifest fixes:
  KDE runtime 6.8 -> 6.10 (6.8 is gone from Flathub), added a libplacebo module (mpv needs it), mpv `-Dx11=disabled` (xpresent missing; Lumina uses the render API).
  Build: `flatpak run org.flatpak.Builder --user --force-clean --disable-rofiles-fuse <dir> packaging/flatpak/io.github.mdmakky.Lumina.yml`.
  `desktop-file-validate` / `appstreamcli validate --no-net` pass. Still missing: Flathub screenshots; sound/MPRIS/portals inside the Flatpak not hand-tested.
- The stale `build/linux-debug` (configured from another path) had to be recreated; if cmake complains about a different source dir, move the build dir aside and re-run `cmake --preset linux-debug`.

**UI changes made from the owner's hands-on feedback (owner confirmed volume + preview work; the rest was built but not re-confirmed)**
1. All menus (`PopupPanel`) are now `modal: true, dim: false`: clicking outside only closes the menu; before it also toggled pause.
2. Auto-hide: `hideTimer` re-arms while a menu/playlist is open or the chrome is hovered (it used to give up and never hide again after the menu closed).
3. Default subtitle size 55 -> 40 (`SettingsDialog.qml`); existing saved sizes are untouched.
4. **Volume model (Linux):** `PlayerController.systemVolume` (-1 = unknown). When the OS backend reports it, the volume slider, Up/Down keys, mouse wheel and the OS volume keys
   all drive/mirror the **OS sink volume** (`pactl`, 0-100), and the mpv volume is forced to 100 (no double attenuation). When it is -1 (macOS, Windows, no pactl)
   everything falls back to the old mpv volume (0-150). Entry point: `PlayerController::setVolumePercent`. Linux code: `MprisIntegration::watchSystemVolume` (`pactl subscribe`)
   and `setSystemVolume`; new `PlatformIntegration::systemVolumeChanged` signal + `setSystemVolume` virtual (no-op default).
   **macOS agent: `systemVolume` stays -1 there, so verify the slider/keys/wheel still drive mpv volume exactly as before. If you want the same "slider = system volume" on macOS, implement it in `MacIntegration.mm`.**
5. Volume OSD: any volume change shows the chrome and a "Volume N%" toast (`Main.qml volumeOsd`). Mute shows the slider at 0; changing volume while muted unmutes.
6. Mouse wheel / touchpad over the video changes volume: accumulated, 1% per 24 angle units (a mouse notch = 5%), direction follows the gesture via `inverted`. Tune the `24` if too sensitive.
7. `VolumeSlider.qml` redesigned (128 px, thin track that thickens on hover, handle only on hover, % bubble, always-visible `NN%`). No `font.features` (Qt 6.4 CI).
8. Seek-preview card (`TimeSlider.qml` tooltip) restyled: neutral near-black card, one hairline border, no inner frame.
9. Video adjustment popup: preset chips Default / Cinema / Vivid / Night (`VideoAdjustPopup.qml`); values are guesses, tune freely.

**Tests:** `ctest` 11/11 passed on Fedora after items 1-9. `tools/run-ui-test.sh` was **not** run (owner asked not to run big suites); please run it once on macOS, since popups are now modal.
Owner also asked what "Always on top" and "Deinterlace" do: top = window above others (may not work on GNOME Wayland, untested); deinterlace = removes comb lines from old interlaced TV/DVD video, useless for modern movies.

## Open work, in priority order
1. Seek smoothness vs VLC on Fedora (VAAPI is confirmed via `LUMINA_SEEK_BENCH`; VLC numbers still missing).
2. Flatpak: hand-test sound, MPRIS, portals, file dialogs; add Flathub screenshots. (Build itself is done.)
3. Verify the new popup panels, Settings and Bengali (install LinguistTools) on Linux; fix whatever looks wrong there.
4. Windows build (needs real Windows), macOS signing, translations review, crash reporting.
5. Seek-preview thumbnails and library features only checked on macOS test patterns — check with real movies.

## macOS session (2026-11-06, later) — UI redesign to verify on Linux
Pushed up to `361696c`. Built and screenshot-checked on macOS only.

- New start screen `ui/Components/HomeView.qml`, redesigned library `ui/Library/LibraryView.qml` (Back button, underline tabs, segmented sort/view, thumbnail rows, card grid).
- Shared surfaces: `Surface.qml`, `ModalCard.qml`, `ModalFooter.qml`; PopupPanel, Open URL, Media info, Settings frame, tooltip and library dialogs use them. Section titles are sentence case via `Theme.sentence()`.
- Icons: `Glyph.qml` draws SVG paths as data URIs in the theme colour (no asset files). macOS logs 5x `qt.svg: Invalid path data; path truncated` at startup though the icons render; find the truncated path.
- Theme: dark accent `#6A5EEA`, `accentText` picked by WCAG contrast, darker panel border, new tokens (`scrimModal`, `videoCard`, `panelHighlight`, font size tokens).
- Window geometry restore is clamped to the screen (`Main.qml`); startup volume toast removed; queue panel hidden on the start screen.

**Check on Linux:** icons render, Home/Library/Settings/Open URL (Ctrl+U)/Media info (Ctrl+I) look right on Qt 6.11 + Wayland, window clamp does not misbehave on GNOME, Surface shadow cost, CI Qt 6.4 still green.
Quick test (not the full suite): `LUMINA_UI_ONLY_HOME_LIBRARY=1 LUMINA_UI_TEST=1 LUMINA_SCREENSHOT=/tmp/shots ./build/linux-debug/lumina /path/to/real.mkv`
Not seen on macOS either: Equalizer, Picture and Bookmarks popup interiors under the new chrome.

## Linux check of the macOS UI redesign (2026-11-06, Fedora, Qt 6.11 + Wayland)
Built `5df9950` cleanly. `LUMINA_UI_ONLY_HOME_LIBRARY=1 LUMINA_UI_TEST=1` passes (exit 0, no FAIL); Home, Library list/grid/tabs/search/chips render correctly (screenshots checked by eye).
- **Gotcha:** that test needs `/tmp/short1.mp4` (a tiny file). Without it the fixture copies the first media argument 3 times — with a 5.7 GB movie it fills a 7.5 GB tmpfs `/tmp` and the library checks fail.
  Make it with: `ffmpeg -f lavfi -i testsrc2=size=1280x720:rate=30 -f lavfi -i sine=f=440 -t 6 -c:v libx264 -pix_fmt yuv420p -c:a aac /tmp/short1.mp4`. (Consider making the fixture refuse files > 50 MB.)
- **`qt.svg: Invalid path data` warning fixed:** it was not a bad path. `Glyph.qml` built an SVG with `d=""` while a bound `name` was still empty at creation. Now `source` stays empty until `d` is set; warnings 5 -> 0 on Linux (all 15 paths parse fine in QtSvg).
- Not checked on Linux: Settings pages, Open URL / Media info dialogs, Equalizer/Picture/Bookmarks popup interiors, window clamp on GNOME, CI Qt 6.4.

## Remaining work — owner will continue on macOS (2026-11-06)
Owner tested the app on macOS and Fedora and says it works ("so far thik ase"). **Windows has never been built.** Decision: ship a **preview** (`v0.3.1-preview`, GitHub pre-release), not a full release: signing, auto-update and crash reporting do not exist yet.

Do on macOS, in this order:
1. **CI first.** Several runs for `361696c`..`4df7b13` were still `in_progress` when this was written. Check `gh run list`; fix any Linux Qt 6.4 failure (known traps: `font.features`, JS reserved words, QStringLiteral). Do not tag until CI is green.
2. **Make the UI-test fixture safe** (`app/UiTest.cpp`, "library fixture media"): it copies the first media argument 3x when `/tmp/short1.mp4` is missing and filled a 7.5 GB tmpfs with a 5.7 GB movie. Refuse files > 50 MB or generate a tiny file instead.
3. **Run `tools/run-ui-test.sh` once** (popups are modal now, volume model changed — see the Linux session section) and `ctest`. Fix only what fails.
4. **macOS volume:** `systemVolume` is -1 on macOS so the slider drives mpv volume. Decide whether to mirror the Linux behaviour (slider = system output volume via CoreAudio in `MacIntegration.mm`; implement `PlatformIntegration::setSystemVolume` + emit `systemVolumeChanged`). Probably not worth it; at least confirm nothing regressed.
5. **macOS integration:** Now Playing / media keys (`MPNowPlayingInfoCenter`, `MPRemoteCommandCenter`) in `MacIntegration.mm`; test the bundle (`tools/macdeploy.sh`, Finder "Open With", launch from the dmg).
6. **Windows:** cannot be built on a Mac. Use the manual Windows CI job (`docs/WINDOWS.md`, MSYS2 package names unverified) to get a first build; expect fixes. Do not claim Windows works until a Windows machine runs it.
7. **Preview release:** write the CHANGELOG entry, tag `v0.3.1-preview`, attach the macOS dmg (ad-hoc signed: Gatekeeper warns, say so in the notes). Create it as a **draft** pre-release; the owner publishes.
8. Still unbuilt/unchecked, needs the owner's accounts or hardware: Developer ID signing + notarization, auto-update, crash reporting, OpenSubtitles.

Back on Fedora later (cannot be done on macOS): VLC seek comparison (`tools/vlc_seekbench.py`, needs `sudo dnf install vlc python3-vlc`), Flatpak hand-test of sound/MPRIS/portals, Flathub screenshots, hand-check of Settings/Open URL/Media info/Equalizer/Picture/Bookmarks on Linux.
