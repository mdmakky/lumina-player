# Built by .github/workflows/packages.yml. Build: rpmbuild -ba packaging/rpm/lumina.spec
# Source0 is a tarball of the git tag: git archive --prefix=lumina-player-0.3.1/ -o ~/rpmbuild/SOURCES/lumina-player-0.3.1.tar.gz v0.3.1-preview
Name:           lumina-player
Version:        0.3.1
Release:        1%{?dist}
Summary:        Modern media player built on Qt 6 and libmpv
License:        GPL-3.0-or-later
URL:            https://github.com/mdmakky/lumina-player
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.28
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig(mpv)
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtbase-private-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  qt6-linguist
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

Requires:       mpv-libs
Requires:       qt6-qtbase-sqlite
Requires:       qt6-qtdeclarative
Requires:       qt6-qtquickcontrols2
Requires:       hicolor-icon-theme
# E-AC3/DTS/H.264 decoding needs RPM Fusion's full FFmpeg; Fedora policy forbids a hard Require.
Recommends:     libavcodec-freeworld
Recommends:     mesa-va-drivers-freeworld
Suggests:       yt-dlp

%description
Lumina Player plays video and audio with mpv's engine inside a clean Qt Quick interface,
with fast seeking, playlists, subtitles, equalizer, a media library and MPRIS support.

%prep
%autosetup -n %{name}-%{version}

%build
%cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/io.github.mdmakky.Lumina.desktop
appstream-util validate-relax --nonet %{buildroot}%{_datadir}/metainfo/io.github.mdmakky.Lumina.metainfo.xml || :

%files
%license LICENSE THIRD_PARTY_NOTICES.md
%doc README.md CHANGELOG.md docs/USER_GUIDE.md
%{_bindir}/lumina
%{_datadir}/applications/io.github.mdmakky.Lumina.desktop
%{_datadir}/metainfo/io.github.mdmakky.Lumina.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/io.github.mdmakky.Lumina.*

%changelog
* Fri Nov 06 2026 mdmakky <makky.cse@gmail.com> - 0.3.1-1
- Preview build
