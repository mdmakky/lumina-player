# PROJECT MASTER SPECIFICATION
# Next-Generation Cross-Platform Media Player

**Document Type:** Product Requirements + Software Architecture + UX Specification + Engineering Execution Plan  
**Project Status:** New Product / Greenfield Development  
**Primary Development Platform:** Linux (Fedora first)  
**Target Platforms:** Linux, Windows, macOS  
**Application Type:** Premium Desktop Multimedia Player  
**Architecture:** Native C++ Core + Qt/QML UI + libmpv Playback Engine  
**Development Philosophy:** Performance First, UX Driven, Modular, Extensible

---

# 1. YOUR ROLE

You are not acting as a code generator or a simple programming assistant.

You are acting as a multidisciplinary senior engineering team consisting of:

- Principal Software Architect
- Senior C++ Systems Engineer
- Multimedia Playback Engine Engineer
- FFmpeg / mpv Integration Specialist
- GPU Rendering and Hardware Acceleration Engineer
- Senior Qt / QML Developer
- Senior UI/UX Product Designer
- Desktop Application Security Engineer
- QA and Performance Engineer
- Cross-platform Release Engineer

Your responsibility is to design, implement, test, and deliver a production-quality desktop media player.

Think independently, challenge bad architectural decisions, identify technical risks, and prioritize long-term product quality.

Do not blindly follow instructions that result in poor engineering decisions.

# 2. PRODUCT VISION

Build a modern, premium, highly customizable, cross-platform multimedia player inspired by the functional strengths of PotPlayer.

The product must combine:

1. PotPlayer-level playback controls and customization.
2. mpv-level playback reliability and rendering capabilities.
3. A modern, polished, premium desktop interface.
4. Excellent subtitle and audio management.
5. Intelligent hardware acceleration.
6. Advanced media processing.
7. A flexible plugin and extension architecture.
8. A consistent experience across Linux, Windows, and macOS.

The application must not be a PotPlayer clone.

Build an original product identity, original interface, original interaction patterns, and maintainable software architecture.

**Core product philosophy:**

"Professional playback capabilities without compromising simplicity, beauty, or performance."

# 3. TARGET USERS

Design the application for four user categories.

### A. Everyday Users
- Open and play media files.
- Watch movies and TV shows.
- Control playback speed.
- Load subtitles.
- Create playlists.
- Resume previous playback.

### B. Advanced Media Users
- Configure video rendering.
- Select hardware decoders.
- Adjust subtitle synchronization.
- Configure audio output.
- Use keyboard shortcuts.
- Customize video processing.

### C. Professional Users
- Inspect media codecs and metadata.
- Monitor playback performance.
- Compare rendering and decoding methods.
- Use frame stepping and precise seeking.
- Configure color management and HDR.

### D. Power Users
- Customize every major control.
- Import/export configurations.
- Use plugins and scripts.
- Create custom keyboard shortcuts.
- Configure multiple playback profiles.

# 4. MARKET POSITIONING

Study the following products before making major product decisions.

| Product | Primary inspiration |
|---|---|
| PotPlayer | Advanced playback controls and customization |
| mpv | Playback engine and rendering |
| VLC | Format compatibility |
| IINA | Modern interface |
| SMPlayer | Advanced player controls |
| Kodi | Media library and extensibility |
| MPC-BE | Traditional advanced desktop playback |

Do not copy their interfaces.

Identify their strongest capabilities and design a coherent product that improves on the overall experience.

Our differentiation:

- Advanced controls without an overwhelming interface.
- Better discoverability of advanced features.
- Reliable seeking and playback recovery.
- Consistent cross-platform experience.
- Strong local-first media management.
- Professional-looking visual design.

# 5. TECHNOLOGY STACK

Use the following recommended stack unless a documented technical limitation justifies a change.

## Core Language
C++20 initially, with C++23 adoption where supported and useful.

Requirements:
- Modern C++ practices.
- RAII.
- Smart pointers.
- Explicit ownership.
- Thread-safe boundaries.
- No unnecessary global state.
- Strict separation of concerns.

## Desktop UI
Qt 6 + Qt Quick + QML.

Use:
- Qt Quick Controls where appropriate.
- Custom QML components.
- C++ backend exposed through deliberate QML interfaces.
- Qt Multimedia only where it provides useful system-level functionality.
- Native OS integration through platform abstractions.

The UI must not contain playback-engine business logic.

## Playback
libmpv embedded through its supported client and render APIs.

Use:
- libmpv for media playback.
- mpv properties and commands for playback control.
- mpv event system for playback state.
- mpv render API for video presentation where supported.
- Separate rendering integration for platform-specific requirements.

Do not launch mpv as an external process as the primary playback architecture.

Do not use native window-handle embedding as the default approach without evaluating the render API.

## Media Processing
Use FFmpeg functionality through the chosen playback stack where possible.

Avoid implementing a second decoding pipeline unnecessarily.

Use FFmpeg libraries directly only when a separate capability genuinely requires them, such as independent metadata probing or thumbnail extraction.

## Video Rendering
Use mpv GPU rendering capabilities and libplacebo where supported.

Evaluate:
- OpenGL
- Vulkan
- Direct3D 11
- Metal through supported platform paths

Do not assume every rendering backend is supported on every OS.

## Database
SQLite.

Use it for:
- Media library metadata.
- Watch history.
- Playback positions.
- Playlists.
- Bookmarks.
- Preferences.
- User profiles.

## Build System
- CMake
- Ninja
- CMakePresets
- Git
- CTest

## Testing
- GoogleTest or Catch2.
- Qt Test for UI-related tests.
- Automated playback test harness.
- Performance benchmark suite.

# 6. APPLICATION ARCHITECTURE

Implement a modular architecture.

### Layer 1: Presentation

Contains:
- Main window.
- Video canvas.
- Playback controls.
- Timeline.
- Playlist.
- Media library.
- Settings.
- Dialogs.
- Notifications.
- Context menus.

Technology: QML / Qt Quick.

### Layer 2: Application Core

Contains:
- PlayerController.
- PlaybackState.
- PlaybackQueue.
- CommandDispatcher.
- EventDispatcher.
- ApplicationLifecycle.
- WindowController.

### Layer 3: Media Engine

Contains:
- MediaEngine interface.
- MpvEngine implementation.
- SeekController.
- AudioController.
- SubtitleController.
- VideoController.
- PlaybackStatistics.
- HardwareAccelerationManager.

### Layer 4: Media Services

Contains:
- MediaProbe.
- MetadataService.
- ThumbnailService.
- MediaDiscovery.
- SubtitleService.
- PlaylistService.

### Layer 5: Persistence

Contains:
- DatabaseManager.
- Repository interfaces.
- SQLite repositories.
- Database migrations.
- SettingsRepository.

### Layer 6: Platform Abstraction

Implement separate adapters for:
- Linux.
- Windows.
- macOS.

Platform adapters must handle:
- File associations.
- Native file dialogs.
- System media keys.
- Window behavior.
- Notifications.
- Power management.
- Hardware capabilities.
- Packaging requirements.

Never put platform-specific code directly inside the shared playback controller.

# 7. COMPLETE FEATURE REQUIREMENTS

Implement features in priority order.

## 7.1 Core Playback

Priority: P0.

- Open single media files.
- Open multiple media files.
- Drag and drop.
- Play/pause.
- Stop.
- Previous/next.
- Seek forward/backward.
- Accurate seeking.
- Fast seeking.
- Playback speed adjustment.
- Frame-by-frame navigation.
- Resume playback.
- Remember last playback position.
- Fullscreen.
- Borderless playback.
- Always on top.
- Window resizing.
- Multiple monitor support.
- Multiple application instances.
- Repeat and shuffle.
- Playback queue.
- Chapter navigation.
- Media information.

## 7.2 Video Controls

Priority: P1.

- Aspect ratio selection.
- Crop.
- Zoom.
- Pan.
- Rotation.
- Horizontal and vertical flip.
- Brightness.
- Contrast.
- Saturation.
- Hue.
- Gamma.
- Video delay.
- Deinterlacing.
- Sharpening.
- Denoising.
- Scaling algorithms.
- Color management.
- HDR handling.
- Screenshot capture.
- Snapshot sequence capture.
- Custom shaders.

Advanced processing must be applied only when supported and must not unnecessarily disable hardware decoding.

## 7.3 Audio System

Priority: P1.

- Multiple audio tracks.
- External audio files.
- Audio synchronization.
- Volume normalization.
- Equalizer.
- Channel selection.
- Stereo.
- Mono.
- 5.1 and 7.1 output where supported.
- Audio passthrough.
- Audio device selection.
- Audio visualization.
- Volume amplification.
- Audio delay.

## 7.4 Subtitle System

Priority: P0.

- SRT.
- ASS.
- SSA.
- VTT.
- SUB.
- IDX/SUB.
- PGS.
- External subtitle loading.
- Automatic subtitle discovery.
- Multiple subtitle tracks.
- Subtitle enable/disable.
- Subtitle delay.
- Subtitle font selection.
- Font size.
- Text color.
- Outline.
- Shadow.
- Background.
- Position.
- Alignment.
- Encoding selection.
- Dual subtitle support.
- Subtitle style presets.

Do not reinvent subtitle rendering if the playback engine already provides mature support.

## 7.5 Advanced Playback

Priority: P2.

- A-B repeat.
- Bookmarks.
- Playback markers.
- Scene navigation.
- Playlist management.
- Picture-in-Picture.
- 360-degree video.
- VR viewing mode.
- 3D side-by-side.
- 3D top-bottom.
- Playback profiles.
- Frame interpolation where technically supported.
- Gapless audio playback.

## 7.6 Media Library

Priority: P2.

Home screen sections:
- Continue Watching.
- Recently Played.
- Recently Added.
- Videos.
- Music.
- Favorites.
- Playlists.
- Folders.

Additional capabilities:
- Folder scanning.
- Manual library management.
- Search.
- Sorting.
- Filtering.
- File metadata.
- Poster and thumbnail display.
- Watch progress.
- Duplicate detection.

Media library must be optional. Users must be able to use the application as a simple file player without indexing their entire disk.

## 7.7 Advanced Utilities

Priority: P2/P3.

- Playback statistics.
- Codec inspector.
- Media information.
- Subtitle file association.
- Keyboard shortcut editor.
- Mouse gesture editor.
- Custom themes.
- Configuration export/import.
- Plugin architecture.
- Scripting API.
- Custom playback profiles.

## 7.8 Streaming

Priority: P3.

Evaluate support for:
- HTTP.
- HTTPS.
- HLS.
- MPEG-DASH.
- RTSP.
- Network shares.
- Supported live streams.

Network streaming must include appropriate buffering, reconnect behavior, timeout handling, and security restrictions.

DRM support is not assumed.

# 8. HARDWARE ACCELERATION AND PERFORMANCE

This is one of the highest-priority product requirements.

A visually attractive player with poor seeking or dropped frames is a failed product.

Implement an intelligent hardware acceleration manager.

### Linux

Evaluate:
- VA-API.
- NVDEC.
- Vulkan Video where available.
- GPU rendering compatibility.
- Wayland and X11 environments.

### Windows

Evaluate:
- D3D11VA.
- DXVA2 where required.
- NVDEC.
- Available GPU rendering backends.

### macOS

Evaluate:
- VideoToolbox.
- Metal-compatible rendering paths.
- Apple Silicon optimization.

### Decoder selection

Provide:
1. Automatic.
2. Hardware acceleration preferred.
3. Software decoding.
4. Advanced manual configuration.

Automatic mode must detect capabilities and use a tested configuration.

If hardware initialization fails:
- Record the reason.
- Fall back safely.
- Notify the user without interrupting unnecessarily.
- Allow advanced diagnostics.

### Seeking engineering

Implement and test:

- Fast keyframe-based seek.
- Accurate seek.
- Seek request cancellation.
- Decoder flush.
- Queue cleanup.
- Audio/video synchronization recovery.
- Rapid repeated seeking.
- Recovery after failed seeks.

Never block the UI thread during seeking or decoding operations.

### Performance diagnostics

Expose:
- Active decoder.
- Active rendering backend.
- Hardware acceleration status.
- Video codec.
- Resolution.
- FPS.
- Dropped frames.
- Estimated bitrate.
- Buffering state.
- Decoder errors.
- Rendering errors.

Use bounded queues and proper resource cleanup.

Avoid copying GPU frames to system memory unnecessarily.

Do not promise zero dropped frames under all circumstances. Optimize and report real performance.

# 9. UI/UX DESIGN SYSTEM

The application must feel like a premium modern desktop product.

## Visual Direction

- Dark-first interface.
- Cinematic video presentation.
- Deep charcoal background.
- Layered surfaces.
- Subtle borders.
- Carefully selected accent color.
- Consistent typography.
- Beautiful icons.
- Restrained animations.
- Clear spacing and hierarchy.

Suggested design tokens:

Background: #101114  
Surface: #191B20  
Elevated Surface: #22252B  
Primary Text: #F4F5F7  
Secondary Text: #A3A7B0  
Accent: #8175F5  
Border: #30333B

Use a centralized theme system. Do not hardcode colors throughout components.

## Main Window

The main window should contain:

- Application title area.
- Video canvas.
- Playback control overlay.
- Seek timeline.
- Volume control.
- Playback speed control.
- Audio track shortcut.
- Subtitle shortcut.
- Fullscreen button.
- More actions menu.

Controls should intelligently hide when the mouse is idle and reappear when needed.

## Home Screen

Create a polished media dashboard with:
- Recent media.
- Continue watching.
- Favorites.
- Playlists.
- Library navigation.
- File opening action.

Do not force the user to use a library.

## Playlist Panel

Support:
- Queue management.
- Drag and drop reordering.
- Remove items.
- Clear queue.
- Save playlist.
- Load playlist.
- Search within playlist.
- Current playing indicator.

## Settings Experience

Use a modern settings window with left navigation and a searchable settings panel.

Categories:

1. General.
2. Playback.
3. Video.
4. Audio.
5. Subtitles.
6. Hardware Acceleration.
7. Rendering.
8. Keyboard and Mouse.
9. Playlist.
10. Media Library.
11. Appearance.
12. Plugins.
13. Network.
14. Advanced.

Provide two configuration levels:

- Basic Settings.
- Advanced Settings.

Advanced settings should explain technical options without exposing confusing internal details to everyday users.

## Animation Guidelines

- Use restrained transitions.
- Avoid unnecessary movement.
- Keep controls responsive.
- Avoid animations that interfere with video rendering.
- Respect reduced-motion preferences.
- Keep menus and dialogs consistent.

# 10. KEYBOARD AND MOUSE INTERACTIONS

Implement configurable keyboard shortcuts.

Suggested defaults:

| Action | Shortcut |
|---|---|
| Play/Pause | Space |
| Seek backward | Left |
| Seek forward | Right |
| Volume up | Up |
| Volume down | Down |
| Mute | M |
| Fullscreen | F |
| Next frame | Period |
| Previous frame | Comma |
| Audio track | A |
| Subtitle track | S |
| Subtitle visibility | W |
| Speed decrease | X |
| Speed increase | C |
| Reset speed | Z |
| Open file | Ctrl+O |
| Preferences | Ctrl+, |
| Playlist | F6 |
| Screenshot | Ctrl+S |

These are suggested defaults, not fixed limitations.

Implement:
- Shortcut customization.
- Shortcut conflict detection.
- Mouse wheel configuration.
- Double-click actions.
- Mouse gestures.
- Context menus.

# 11. DATABASE DESIGN

Use SQLite.

Initial tables:

- media_files
- library_sources
- playback_history
- watch_progress
- playlists
- playlist_items
- bookmarks
- settings
- subtitle_preferences
- audio_preferences
- user_profiles

Requirements:
- Database migrations.
- Indexed queries.
- Transaction handling.
- Backup and recovery.
- Corruption detection.
- Safe schema upgrades.

Do not store media binaries inside SQLite.

Playback must continue working if the database is unavailable.

# 12. FILE FORMAT COMPATIBILITY

Target broad media compatibility.

### Containers
MKV, MP4, MOV, AVI, WMV, WebM, FLV, TS, M2TS, MPEG.

### Video codecs
H.264, H.265/HEVC, AV1, VP9, VP8, MPEG-2, MPEG-4, ProRes.

### Audio codecs
MP3, AAC, FLAC, ALAC, WAV, Opus, Vorbis, AC3, E-AC3, DTS.

### Subtitle formats
SRT, ASS, SSA, VTT, SUB, IDX/SUB, PGS.

### Playlist formats
M3U, M3U8, PLS, CUE.

Treat this as a compatibility target, not an unconditional guarantee.

Document format-specific limitations and hardware-dependent support.

# 13. PROJECT STRUCTURE

Start with a maintainable structure similar to:

media-player/
├── CMakeLists.txt
├── CMakePresets.json
├── README.md
├── LICENSE
├── THIRD_PARTY_NOTICES.md
├── app/
│   ├── main.cpp
│   └── ApplicationContext.cpp
├── src/
│   ├── core/
│   ├── engine/
│   ├── media/
│   ├── services/
│   ├── storage/
│   ├── platform/
│   │   ├── linux/
│   │   ├── windows/
│   │   └── macos/
│   └── utilities/
├── ui/
│   ├── Main.qml
│   ├── Components/
│   ├── Player/
│   ├── Library/
│   ├── Playlist/
│   ├── Settings/
│   ├── Dialogs/
│   └── Theme/
├── resources/
│   ├── icons/
│   ├── fonts/
│   ├── shaders/
│   └── translations/
├── plugins/
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── playback/
│   ├── performance/
│   └── ui/
└── packaging/
    ├── linux/
    ├── windows/
    └── macos/

Adjust this structure where justified, but maintain module boundaries.

# 14. SECURITY AND PRIVACY

The application must be local-first.

Requirements:
- No mandatory account.
- No mandatory cloud connection.
- No telemetry without explicit consent.
- No automatic media uploads.
- No automatic execution of untrusted scripts.
- Safe parsing of media metadata.
- Restrict network protocols appropriately.
- Protect configuration and plugin loading.
- Handle malformed media files safely.
- Do not run the application with unnecessary elevated privileges.

Plugin execution must have documented permissions and security boundaries.

# 15. TESTING STRATEGY

Create a reusable reference media test suite.

Test:
- 1080p H.264.
- 4K HEVC 10-bit.
- AV1.
- 4K 60 FPS.
- HDR10.
- HLG.
- Multiple audio tracks.
- Multiple subtitles.
- ASS animated subtitles.
- Variable frame rate.
- Long-duration media.
- Damaged media.
- Very large files.
- Network streams.
- Rapid seeking.
- Repeated play/pause.
- Audio device switching.
- Display switching.
- Sleep and wake recovery.

### Cross-platform testing

Linux:
- Fedora.
- Ubuntu.
- At least one additional distribution.
- Wayland and X11 where available.

Windows:
- Windows 10/11 supported configurations.
- Integrated and discrete GPUs.

macOS:
- Apple Silicon.
- Intel compatibility where supported.

### Performance benchmarks

Measure:
- Cold startup.
- Time to first frame.
- Seek response time.
- Seek completion time.
- Dropped frames.
- CPU utilization.
- Memory utilization.
- GPU utilization where measurable.
- Long playback stability.

Compare against mpv and VLC using identical hardware and test files.

Do not fabricate performance results.

# 16. RELEASE AND DISTRIBUTION

Linux:
- Flatpak.
- RPM.
- AppImage.
- DEB.
- Arch packaging.

Windows:
- Installer.
- Portable build.

macOS:
- DMG.
- Signed and notarized application.

Build reproducibly.

Include:
- Versioning.
- Release notes.
- Dependency notices.
- Crash diagnostics.
- Update mechanism.
- Installation instructions.
- Uninstallation support.

# 17. LICENSING AND THIRD-PARTY COMPLIANCE

Before distribution:

- Audit libmpv licensing.
- Audit FFmpeg configuration and licensing.
- Audit Qt modules and licensing.
- Audit all third-party dependencies.
- Track licenses in an SBOM.
- Include required notices.
- Document source-code and relinking obligations.

Do not assume that using open-source dependencies automatically permits proprietary distribution.

If commercial closed-source distribution is a future requirement, evaluate all dependency obligations before finalizing the build architecture.

# 18. DEVELOPMENT ROADMAP

Build the product in milestones.

## Milestone 1 — Foundation

Deliver:
- Repository.
- CMake configuration.
- Qt application bootstrap.
- Main window.
- Theme system.
- Core module boundaries.
- Logging.
- Basic CI.

Acceptance:
The application builds and opens on Fedora.

## Milestone 2 — Playback Engine

Deliver:
- libmpv integration.
- Video rendering.
- Audio playback.
- Play/pause.
- File opening.
- Seeking.
- Playback state.
- Error reporting.

Acceptance:
Common video and audio files play reliably.

## Milestone 3 — Core UX

Deliver:
- Modern player controls.
- Timeline.
- Fullscreen.
- Playlist.
- Volume.
- Playback speed.
- Keyboard shortcuts.
- Drag and drop.

Acceptance:
The application feels like a usable, polished media player.

## Milestone 4 — Subtitle and Audio

Deliver:
- Subtitle management.
- External subtitles.
- Subtitle synchronization.
- Audio track selection.
- Audio synchronization.
- Audio settings.

Acceptance:
Users can comfortably watch multi-track and subtitle-heavy content.

## Milestone 5 — Performance

Deliver:
- Hardware detection.
- Decoder selection.
- Fallback logic.
- Seek optimization.
- Statistics.
- Benchmark suite.

Acceptance:
Playback and seeking are measured against reference players.

## Milestone 6 — Advanced Features

Deliver:
- Video processing.
- Equalizer.
- Bookmarks.
- A-B repeat.
- Advanced settings.
- Configuration profiles.

## Milestone 7 — Media Library

Deliver:
- Folder scanning.
- Metadata.
- Watch history.
- Thumbnails.
- Search.
- Favorites.

## Milestone 8 — Cross-platform

Deliver:
- Windows build.
- macOS build.
- Platform adapters.
- OS integration.
- Packaging.
- Hardware compatibility testing.

## Milestone 9 — Release Preparation

Deliver:
- Security review.
- License review.
- QA report.
- Performance report.
- User documentation.
- Installation packages.

# 19. NON-NEGOTIABLE ENGINEERING RULES

1. Never block the UI thread with decoding or long-running operations.
2. Never create a custom codec implementation when a mature supported implementation exists.
3. Never mix UI state with media-engine state.
4. Never put all functionality in one giant player controller.
5. Never hardcode OS-specific behavior in shared modules.
6. Never ignore memory leaks or unbounded queues.
7. Never silently swallow playback errors.
8. Never claim a feature works without testing it.
9. Never use placeholder buttons in a completed feature.
10. Never sacrifice playback reliability for animations.
11. Never implement advanced features by duplicating existing engine capabilities unnecessarily.
12. Never make the media library mandatory.
13. Never bundle dependencies without license review.
14. Never declare cross-platform support based solely on a successful Linux build.

# 20. AGENTIC DEVELOPMENT WORKFLOW

You have permission to investigate, design, implement, refactor, test, and document the project.

However, follow this workflow:

### Step A: Repository Investigation

- Inspect the existing repository.
- Identify available tools and installed dependencies.
- Inspect existing code before making architectural changes.
- Determine the actual host operating system.
- Identify GPU and hardware capabilities when available.
- Identify missing dependencies.

Do not assume the development machine's hardware.

### Step B: Architecture Validation

Before extensive implementation:
- Validate Qt/libmpv integration.
- Validate video rendering.
- Validate seeking.
- Validate hardware decoding.
- Validate packaging feasibility.
- Document important risks.

### Step C: Build a Real Playback Prototype

Before implementing the complete UI, prove that:
- A video file opens.
- Audio works.
- Video renders.
- Seeking works.
- Playback resumes after seeking.
- Subtitles work.
- Hardware decoding can be detected.
- Playback errors are handled.

### Step D: Build the Product UI

Create the design system first.

Then implement:
- Main player.
- Playback controls.
- Playlist.
- Settings.
- Library.

Do not produce a generic default Qt application interface.

### Step E: Validate Every Milestone

For each milestone:
- Build.
- Run.
- Test.
- Fix.
- Document.
- Commit logically when version control is available.

Do not claim completion based on code generation alone.

### Step F: Maintain Project Documentation

Maintain:
- README.md
- ARCHITECTURE.md
- PRODUCT_REQUIREMENTS.md
- TECHNICAL_DECISIONS.md
- BUILDING.md
- TESTING.md
- ROADMAP.md
- THIRD_PARTY_NOTICES.md

Keep documentation synchronized with actual implementation.

# 21. FIRST EXECUTION INSTRUCTION

Start with repository investigation and a minimal architecture validation.

Do not spend excessive time writing another theoretical plan.

Do not attempt to implement all features simultaneously.

First prove the playback engine and rendering integration.

Then build a polished, working core player.

After that, implement features incrementally according to the milestones.

Your first deliverable must contain:

1. Repository assessment.
2. Architecture decision record.
3. Dependency and licensing assessment.
4. Actual development environment assessment.
5. Minimal Qt + libmpv playback prototype.
6. Successful build instructions.
7. Playback test results.
8. Next implementation steps.

If the repository is empty, bootstrap the project.

If it contains an existing implementation, inspect and preserve useful work.

Ask questions only when an important decision cannot reasonably be made from available information.

Prefer sensible engineering defaults.

# 22. FINAL PRODUCT SUCCESS CRITERIA

The project is successful when:

- It works as a reliable everyday media player.
- It provides advanced playback controls.
- It delivers excellent subtitle and audio experiences.
- It supports hardware acceleration where available.
- Seeking feels responsive and predictable.
- Its interface looks and feels like a modern premium product.
- It can be built and distributed on Linux, Windows, and macOS.
- Its architecture supports future features without a complete rewrite.
- Its documentation and tests reflect the actual product.

**Final instruction: Build a genuinely excellent media player, not a feature checklist disguised as software.**

Prioritize playback correctness, interaction quality, visual polish, and long-term maintainability.