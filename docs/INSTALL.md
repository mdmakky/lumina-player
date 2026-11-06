# Installing Lumina Player

Lumina Player is in **preview** (0.3.1-preview). All downloads are on the
[Releases page](https://github.com/mdmakky/lumina-player/releases).

## Pick your package

| You use | Download | Install |
|---|---|---|
| **Any Linux** with Flatpak | `Lumina-Player-<version>.flatpak` | `flatpak install --user ./Lumina-Player-*.flatpak` |
| **Fedora 43 / 44 / 45** | `lumina-player-<version>.fc<N>.x86_64.rpm` (N = your release) | `sudo dnf install ./lumina-player-*.rpm` |
| **Ubuntu 24.04 / 25.10 / 26.04** | `lumina-player_<version>_ubuntu<N>_amd64.deb` (N = your release) | `sudo apt install ./lumina-player_*.deb` |
| **macOS** (Apple Silicon) | `Lumina-Player-<version>-macOS.dmg`, built on a Mac and attached to a release when ready (it may lag the Linux packages; if the release has none yet, build from source) | open, drag to Applications |
| **Windows** | not available yet | see [WINDOWS.md](WINDOWS.md) to build it yourself |

Check your release first: `rpm -E %fedora` (Fedora) or `lsb_release -rs` (Ubuntu).
An RPM or DEB is built against the Qt libraries of **one** distro release, so the
file for another release will not install. If your distro is not in the table, use the Flatpak.

## What has been tested

| Package | Automated (every release, in CI) | On real hardware |
|---|---|---|
| Fedora RPM | clean Fedora 43, 44, 45 container: the package installs, then the app starts, loads a video, seeks, pauses and resumes | Fedora 45 (GNOME, Wayland, AMD GPU) |
| Ubuntu DEB | same, on clean Ubuntu 24.04, 25.10, 26.04 containers | not yet (container only) |
| Flatpak | built in CI | Fedora 45 (video, sound, VAAPI hardware decoding) |
| macOS DMG | built on a Mac with `tools/macdeploy.sh` | Apple Silicon Mac |
| Windows | not tested | not tested |

The container check runs with a virtual display, software rendering and no audio device,
so it proves "installs and plays", not "looks right on your GPU". Please report problems:
[open an issue](https://github.com/mdmakky/lumina-player/issues/new/choose).

## Linux

### Flatpak (any distribution)

Needs [Flatpak](https://flatpak.org/setup/). The bundle adds Flathub as the source of the KDE
runtime it needs, so the first install is a large one-time download.

```bash
flatpak install --user ./Lumina-Player-*.flatpak
flatpak run io.github.mdmakky.Lumina /path/to/video.mkv
```

The bundled FFmpeg includes all native decoders (E-AC3, DTS, H.264, HEVC, ...), so no codec
packages are needed. The app can read `~/Videos`, `~/Music` and `~/Downloads`; use the
file dialog (or `flatpak override --user --filesystem=/path io.github.mdmakky.Lumina`)
for other folders.

### Fedora (RPM)

```bash
cd ~/Downloads
sudo dnf install ./lumina-player-0.3.1-1.fc45.x86_64.rpm     # use the file for your release
```

Install from a terminal. GNOME Software can open the file too, but the package is **not
signed**, so it asks for an administrator password and labels it "Potentially unsafe".

**Codecs.** Fedora's own FFmpeg cannot decode E-AC3, DTS or software H.264. Enable
[RPM Fusion](https://rpmfusion.org/Configuration) and install the full codecs:

```bash
sudo dnf install https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-$(rpm -E %fedora).noarch.rpm
sudo dnf install libavcodec-freeworld
```

For hardware video decoding on AMD or Intel GPUs also install `mesa-va-drivers-freeworld`
(RPM Fusion; it replaces `mesa-va-drivers`: `sudo dnf swap mesa-va-drivers mesa-va-drivers-freeworld`).
Press `I` in the player to see whether `vaapi` is in use.

### Ubuntu (DEB)

```bash
cd ~/Downloads
sudo apt install ./lumina-player_0.3.1-1_ubuntu24.04_amd64.deb   # use the file for your release
```

Not supported: Ubuntu 22.04 and Debian 12 (their Qt is older than the required 6.4), and
other Debian-based releases that have not been tested. Use the Flatpak there.

## macOS

1. Open the `.dmg` and drag **Lumina** to Applications.
2. The app is only ad-hoc signed (no Apple Developer ID yet), so Gatekeeper blocks the first
   launch. Right-click the app, choose **Open**, then **Open** again. If macOS still refuses:
   System Settings, Privacy & Security, scroll down, **Open Anyway**.

## Windows

There is no supported Windows build yet. An experimental build exists in the CI workflow but has
never been run on a Windows machine. See [WINDOWS.md](WINDOWS.md) if you want to build and test it.

## Verify your download

Each release includes `SHA256SUMS.txt`:

```bash
sha256sum -c SHA256SUMS.txt --ignore-missing
```

## Where your data lives

| | Linux (native) | Linux (Flatpak) |
|---|---|---|
| Settings | `~/.config/mdmakky/Lumina Player.conf` | `~/.var/app/io.github.mdmakky.Lumina/config/` |
| History, favorites, library | `~/.local/share/mdmakky/Lumina Player/library.db` | `~/.var/app/io.github.mdmakky.Lumina/data/` |
| Logs and crash reports | `~/.local/share/mdmakky/Lumina Player/logs/` | under the same `data/` folder |

Nothing is sent anywhere. The only network access is playing a URL you open and the optional
update check (Settings, About), which asks the GitHub releases API and downloads nothing.

## Update

Install the newer package over the old one (`dnf`/`apt`/`flatpak install` upgrade in place).
Your settings and library are kept.

## Uninstall

```bash
sudo dnf remove lumina-player                                  # Fedora
sudo apt remove lumina-player                                  # Ubuntu
flatpak uninstall --user io.github.mdmakky.Lumina              # Flatpak (add --delete-data to remove settings)
```

On macOS drag the app to the Trash. Delete the folders in the table above to remove your data.

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `nothing provides ...` (dnf) or `unmet dependencies` (apt) | You downloaded the file for a different release. Use the one matching `rpm -E %fedora` / `lsb_release -rs`, or the Flatpak. |
| GNOME Software says "Unable to install" | Install from the terminal (see above). Packages are unsigned, so GNOME Software asks for a password; a file for the wrong release fails with a dependency error. |
| The app does not start | Run `lumina` in a terminal and read the message. Logs: Settings, **Open logs folder**. Include the log in a bug report. |
| "No sound" banner | Fedora: install `libavcodec-freeworld` (see Codecs). Also check that PipeWire or PulseAudio is running. |
| Black video or OpenGL errors | mpv's render API needs desktop OpenGL. See the troubleshooting list in [BUILDING.md](BUILDING.md). |
| Buttons show no icons | The Qt SVG image plugin is missing. Install `qt6-svg-plugins` (Ubuntu 25.10 and later) or `libqt6svg6` / `qt6-qtsvg` (installed automatically unless you disabled recommended packages). |
| Hardware decoding is off | See the Fedora note on `mesa-va-drivers-freeworld`; check the `I` overlay in the player. |

## Build from source

See [BUILDING.md](BUILDING.md).
