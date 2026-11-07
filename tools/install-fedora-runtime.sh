#!/bin/sh
# For people who installed the Lumina RPM: adds the codecs Fedora cannot ship.
# Fixes "No sound" on E-AC3/DTS movies and software-only H.264, and enables AMD/Intel GPU decoding.
# Run from a normal terminal (sudo must be allowed). Not needed for the Flatpak.
set -e
FEDORA="$(rpm -E %fedora)"

if ! rpm -q rpmfusion-free-release >/dev/null 2>&1; then
  echo ">> Enabling RPM Fusion (free)"
  sudo dnf install -y "https://mirrors.rpmfusion.org/free/fedora/rpmfusion-free-release-${FEDORA}.noarch.rpm"
fi

echo ">> Full FFmpeg codec set (E-AC3, DTS, H.264)"
sudo dnf install -y libavcodec-freeworld

echo ">> Hardware video decoding (VAAPI) for AMD / Intel"
if rpm -q mesa-va-drivers >/dev/null 2>&1 && ! rpm -q mesa-va-drivers-freeworld >/dev/null 2>&1; then
  sudo dnf swap -y mesa-va-drivers mesa-va-drivers-freeworld || echo "(swap not possible on this release; software decoding still works)"
fi
sudo dnf install -y libva-utils || true

echo
echo "Done. Restart Lumina. Press I while a video plays: the hwdec line should say vaapi."
echo "Check the GPU decoder list with: vainfo"
