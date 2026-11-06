# macOS packaging notes

Build the dmg: `tools/macdeploy.sh` -> `build/mac-bundle/bin/lumina.app` and `build/mac-bundle/Lumina-Player.dmg`
(`Info.plist` comes from `Info.plist.in`; libmpv and its Homebrew dependency tree are copied into `Contents/Frameworks`).

Release steps that need the owner's Apple Developer account (not done):
1. Sign inside-out with a Developer ID Application certificate and hardened runtime:
   `codesign --force --deep --options runtime --timestamp --sign "Developer ID Application: NAME (TEAMID)" lumina.app`
   (sign each dylib in Frameworks first if `--deep` is rejected).
2. Notarize: `xcrun notarytool submit Lumina-Player.dmg --keychain-profile lumina --wait`, then `xcrun stapler staple Lumina-Player.dmg`.
3. Verify on a clean Mac: `spctl -a -vv lumina.app` and a first launch from a downloaded dmg.
4. Universal binary: Homebrew is per-architecture; build once on arm64 and once on x86_64 and `lipo` them, or ship arm64 only.
5. Attach `THIRD_PARTY_NOTICES.md` and the SBOM (`tools/make-sbom.sh`) to the release; FFmpeg/Qt stay as dylibs/frameworks (LGPL relinking).

Known issues: macdeployqt prints rpath errors for QML plugins whose Qt modules are absent (harmless); bundled-app launch is untested.
