# Fedora repository (COPR)

A COPR project gives Fedora users `dnf` install and updates, and COPR signs the packages itself, so the
"Potentially unsafe" warning of a hand-downloaded RPM goes away. It needs a Fedora account (free), so it has to be set up by the project owner.

## One-time setup
1. Sign in at https://copr.fedorainfracloud.org with a Fedora Account (FAS).
2. New project: name `lumina-player`, chroots `fedora-43-x86_64`, `fedora-44-x86_64`, `fedora-45-x86_64` (add `fedora-rawhide-x86_64` if wanted).
3. RPM Fusion is not needed to build. `libavcodec-freeworld` is only a runtime `Recommends:`.
4. Add a package with source type **SCM**: clone URL `https://github.com/mdmakky/lumina-player.git`, committish `main` (or a tag), **Build SRPM with: make srpm**, spec path `packaging/rpm/lumina.spec`, Makefile path `.copr/Makefile`.
5. Optional: enable the GitHub webhook on the package page so every tag builds automatically.

## Users
```bash
sudo dnf copr enable mdmakky/lumina-player
sudo dnf install lumina-player
tools/install-fedora-runtime.sh        # codecs and GPU decoding, needs RPM Fusion (see docs/INSTALL.md)
```

`.copr/Makefile` builds the source RPM from `git archive` of the checkout (the same recipe CI uses). Not yet exercised: it has never run on COPR.
