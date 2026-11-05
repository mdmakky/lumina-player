# Third-Party Notices

Lumina Player depends on the following third-party software. This file summarizes
obligations; a full SBOM review is required before any release distribution (see
`docs/MASTER_SPEC.md` §17).

| Component | License | How we use it |
|---|---|---|
| Qt 6 (Core, Gui, Quick, QuickControls2) | LGPL-3.0 / GPL-3.0 (open-source editions) | UI framework. Dynamically linked. |
| libmpv / mpv | GPL-2.0-or-later (depending on build configuration; typically GPL-3.0+ builds) | Playback engine. Dynamically linked. |
| FFmpeg (used inside mpv builds) | LGPL-2.1+ / GPL depending on configuration | Consumed via libmpv only; not linked directly. |
| libva | MIT | Hardware acceleration probing (VAAPI). |

## Obligations summary

- **LGPL components (Qt open source):** users must be able to relink the application
  with modified library versions. We ship dynamically linked binaries and must provide
  or point to obtainable Qt sources.
- **GPL components (mpv):** distributing binaries requires the complete corresponding
  source of our application under a GPL-compatible license. Lumina Player is GPL-3.0-or-later,
  which satisfies this.
- Qt and mpv logos/branding are not used to imply endorsement.
- Required license texts must accompany any binary distribution.

## Attribution

- mpv: https://mpv.io
- FFmpeg: https://ffmpeg.org
- Qt: https://www.qt.io
- libVA: https://github.com/intel/libva
