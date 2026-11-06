# Changelog

All notable changes are listed here. The format follows [Keep a Changelog](https://keepachangelog.com/).
Versions marked "preview" are pre-releases.

## [0.3.1-preview] - 2026-11-06

First public preview. Linux (Flatpak, Fedora RPM, Ubuntu DEB). The macOS DMG (ad-hoc signed) is built on a Mac and attached after the Linux packages. Windows is not released.

### Added
- Start screen and media library: video and music tabs, search, tags, thumbnails, duplicate detection, playlists, favorites
- Network streams: Open URL (Ctrl+U), yt-dlp for page URLs, buffering, per-file playback profiles
- Seek-bar hover thumbnails; keyframe scrubbing while dragging the timeline
- Linux: MPRIS media controls, screensaver inhibit, volume follows the system volume (slider, keys, mouse wheel, laptop keys)
- Volume bar with live percentage, volume overlay, mute shows 0, mouse wheel and touchpad volume
- Video adjustment presets (Default, Cinema, Vivid, Night)
- Subtitle styling (including Bengali fonts), track labels with language, codec and layout
- Bengali translation with live language switching; Dark, Light and OLED themes; rebindable shortcuts and mouse actions
- Local crash reports (`logs/crash-*.txt`, nothing is sent) and an opt-in update check (Settings, About; at most once a day; downloads nothing)
- Rotating log files and **Open logs folder**; "No sound" banner with a distro-specific fix
- Packages: Flatpak bundle, one RPM per Fedora release (43, 44, 45), one DEB per Ubuntu release (24.04, 25.10, 26.04).
  CI installs each in a clean container of its release and plays a test video before it is attached
- Documentation: install guide, contributing guide, security policy, issue and pull request templates

### Changed
- Menus and dialogs share one modal system; clicking outside a menu only closes it
- Controls hide after the idle delay even if the cursor rests on them
- Default subtitle size is smaller (40)
- A drive that is not mounted keeps its library entries; a damaged library database is set aside and recreated
- `.ts` files under 1 MB are no longer treated as video (TypeScript and Qt Linguist sources)
- The update check skips pre-releases when you run a stable version

### Fixed
- The app did not start on Qt 6.4 (Ubuntu 24.04): QML resource prefix, `Overlay`, cyclic singleton import, `font.features`
- `lumina file.mkv` with a relative path did not open the file
- Controls never hid while a video played (the pointer position was re-sent every frame)
- RPM depended on packages that do not exist on Fedora; DEB lacked the QML Dialogs, Models and SVG packages
- Upside-down video, Qt 6.4 compatibility issues, startup volume flash

### Security
- mpv option deny-list, URL scheme allow-list for files and links from outside the app, private data directory and settings file permissions (see `docs/SECURITY_REVIEW.md`)

### Not included yet
Developer ID signing and notarization, automatic update install, a signed RPM, OpenSubtitles download, a Windows build, 360/VR/3D.

## [0.3.0] - not released separately

- New floating-overlay player UI, home screen with Continue watching / Recent / Favorites
- Playlist: drag-reorder, shuffle, repeat, search, M3U save/load, auto-advance, resume position
- Chapters (timeline markers, menu, PgUp/PgDn), A-B loop, bookmarks, screenshots
- Audio/subtitle track menu, external subtitles, delay controls, subtitle style, 10-band equalizer
- Settings dialog (10 pages, search, per-page reset), rebindable shortcuts, Dark/Light/OLED themes, reduced motion
- Mini-player mode, single-instance, optional media library
- Performance: seek/cache tuning, seek coalescing + keyframe scrubbing, stats overlay, Linux hwdec ladder
- macOS: coreaudio output (fixes ~4 s speed-change lag), bundle + dmg script
- Packaging: desktop file, AppStream metainfo, Flatpak manifest (unbuilt), CI for Linux (Qt 6.4) and macOS
