# Flatpak hand-test checklist

The Flatpak is built and the first checks (video, sound, VAAPI) pass on Fedora 45. These need a person; each takes a minute.
Install: `flatpak install --user ./Lumina-Player_*_Any-Linux-Flatpak.flatpak`, run: `flatpak run io.github.mdmakky.Lumina`.

| # | Check | How | Expected |
|---|---|---|---|
| 1 | Start | run it | Start screen, no error in the terminal |
| 2 | File dialog | Open File, pick a movie from `~/Videos` | plays |
| 3 | Other folder | open a movie from outside Videos/Music/Downloads through the dialog | plays (portal grants access) |
| 4 | Drag and drop | drop a file on the window | plays |
| 5 | Sound | any video with sound; also an E-AC3/DTS movie | audible, no "No sound" banner |
| 6 | GPU decode | press `I` during playback | hwdec line says `vaapi` |
| 7 | Media keys | play/pause key, headset button | toggles playback |
| 8 | MPRIS widget | open the GNOME quick settings media card | shows title, play/pause works |
| 9 | Volume | slider, mouse wheel, keyboard volume keys | OS volume follows (needs `pactl` in the sandbox; if not, mpv volume is used) |
| 10 | Screensaver | play a video for the idle timeout | screen does not blank, blanks again after pause |
| 11 | Screenshot | press `S` | PNG saved in Pictures (or the folder you chose) |
| 12 | Subtitles | load an external .srt/.ass | shown, Bengali text renders |
| 13 | Library | Library, Add folder, scan | items and thumbnails appear |
| 14 | Open URL | Ctrl+U, paste an http video link | plays |
| 15 | Restart | close and reopen | window size and "Continue watching" kept |

If a row fails, note the number and send `~/.var/app/io.github.mdmakky.Lumina/data/mdmakky/Lumina Player/logs` plus the terminal output.
