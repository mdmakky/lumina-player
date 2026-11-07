#!/bin/sh
# One-shot dependency installer for building and running Lumina on Fedora.
# Run from a normal terminal (not a sandboxed IDE terminal: sudo must be allowed).
set -e
FEDORA="$(rpm -E %fedora)"

echo ">> RPM Fusion (needed for E-AC3/DTS/H.264 decoders and AMD VAAPI)"
if ! rpm -q rpmfusion-free-release >/dev/null 2>&1; then
  sudo dnf install -y "https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORA}.noarch.rpm"
fi

echo ">> Build dependencies"
sudo dnf install -y cmake ninja-build gcc-c++ pkgconf-pkg-config \
  qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtsvg-devel qt6-qttools-devel qt6-linguist \
  qt6-qtbase-private-devel mpv-devel

echo ">> Runtime codecs (full FFmpeg codec set for the system libavcodec) + hardware decode"
sudo dnf install -y libavcodec-freeworld libva-utils || true
if rpm -q mesa-va-drivers >/dev/null 2>&1 && ! rpm -q mesa-va-drivers-freeworld >/dev/null 2>&1; then
  sudo dnf swap -y mesa-va-drivers mesa-va-drivers-freeworld || echo "(swap not possible; software decoding still works)"
fi

echo ">> Optional: mpv CLI + yt-dlp (network streams / page URLs)"
sudo dnf install -y mpv yt-dlp || true

echo
echo "Done. Build with:  cmake -S . -B build/linux-debug -G Ninja && cmake --build build/linux-debug"
