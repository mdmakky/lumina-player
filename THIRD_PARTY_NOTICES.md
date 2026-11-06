# Third-Party Notices

Lumina Player (GPL-3.0-or-later) depends on the software below. Machine-readable list: `tools/make-sbom.sh`
(example output: `docs/sbom.example.json`, CycloneDX 1.5). Regenerate it from the exact build environment for each release.

| Component | License | How we use it |
|---|---|---|
| Qt 6 (Core, Gui, Quick, QuickControls2, Sql, DBus, Network, ...) | LGPL-3.0-only / GPL-3.0-only (open-source edition; some modules GPL-only) | UI framework. Dynamically linked (shared libraries / frameworks). |
| libmpv / mpv | GPL-2.0-or-later, or LGPL-2.1-or-later when built with `-Dgpl=false` | Playback engine. Dynamically linked. |
| FFmpeg (libavcodec, libavformat, ...) | LGPL-2.1-or-later; GPL-2.0-or-later if built with GPL options (x264, x265, ...) | Used through libmpv only; bundled in Flatpak/macOS packages. |
| libass | ISC | Subtitle rendering, inside mpv. |
| libplacebo | LGPL-2.1-or-later | GPU video processing, inside mpv (Flatpak builds it). |
| libva | MIT | VAAPI hardware decode, via mpv/FFmpeg. |
| SQLite | Public domain (blessing) | History/library database via Qt SQL driver. |
| yt-dlp (optional, not shipped) | Unlicense | Run by mpv as an external process if the user installed it. |
| Bundled dependencies of the above (HarfBuzz, FreeType, FriBidi, zlib, ...) | MIT / FTL / LGPL / zlib | Pulled in transitively; listed in the SBOM of a packaged build. |

## Icons and artwork
All SVGs under `resources/icons/` (UI glyphs such as play, pause, volume, settings pages) and the application icon
`resources/icons/app/lumina.svg` are original works created in this repository (single author in git history, simple geometric paths,
no third-party icon set imported) and are covered by the project license, GPL-3.0-or-later. No Qt, mpv or VLC logos are used.
Audit date 2026-11-06 was by inspection of `git log` and file content; if any icon was traced from an external set, replace it before release.

## GPL-3.0 compatibility reasoning
- Lumina is GPL-3.0-or-later. mpv's code is GPL-2.0-or-later: "or later" lets us combine it under GPL-3.0. (A GPL-2.0-only library would NOT be compatible; none is used.)
- Qt open-source modules are LGPL-3.0 (or GPL-3.0): both are compatible with GPL-3.0; LGPL-3.0 explicitly permits combination with GPL-3.0 code. Qt modules that are GPL-3.0-only are also fine.
- FFmpeg: LGPL-2.1+ is compatible with GPL-3.0 (upgrade permitted by the LGPL); GPL-2.0-or-later parts are compatible. Builds with `--enable-nonfree` must never be distributed (fdk-aac); the Flatpak and Homebrew builds do not use it.
- libass (ISC), libva (MIT), libplacebo (LGPL-2.1+), SQLite (public domain) are all GPL-3.0-compatible.
- Patent encumbered codecs (H.264/HEVC/AAC) are a legal topic separate from copyright licensing; Fedora's split into `ffmpeg-free` and RPM Fusion exists for this reason. Lumina ships no codecs itself in distro packages; the Flatpak and macOS bundles include FFmpeg's decoders.

## Obligations checklist for a binary release
- [ ] Ship the Lumina source (or a written offer / link at the exact tag) and `LICENSE`.
- [ ] Ship license texts of Qt (LGPL-3.0 + GPL-3.0), mpv, FFmpeg, libass, libplacebo, libva (include `THIRD_PARTY_NOTICES.md` in the package).
- [ ] LGPL relinking: Qt and FFmpeg stay shared libraries (macOS: inside `Contents/Frameworks`, Flatpak: separate `.so` files), and the Qt/FFmpeg/mpv source versions used are stated (point at upstream tarballs, which `packaging/flatpak` already pins).
- [ ] No Qt/mpv/VLC branding implying endorsement.
- [ ] Re-run `tools/make-sbom.sh` on the release build and attach it.

## Attribution
mpv https://mpv.io · FFmpeg https://ffmpeg.org · Qt https://www.qt.io · libass https://github.com/libass/libass ·
libplacebo https://code.videolan.org/videolan/libplacebo · libVA https://github.com/intel/libva · SQLite https://sqlite.org
