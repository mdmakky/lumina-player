pragma Singleton
import QtQuick

// Centralized design tokens (spec §9). Components must never hardcode colors —
// reference the theme instead. Dark-first, cinematic palette.
QtObject {
    // "dark" (default), "light" or "oled". Set from settings at startup and by
    // Settings > Interface; every color below re-binds when it changes.
    property string mode: "dark"
    readonly property bool isLight: mode === "light"
    function pick(dark, light, oled) { return mode === "light" ? light : (mode === "oled" ? oled : dark) }

    // Surfaces
    readonly property color background: pick("#101114", "#F3F4F7", "#000000")
    readonly property color surface: pick("#191B20", "#FFFFFF", "#0A0A0C")
    readonly property color elevated: pick("#22252B", "#E7E9EF", "#15161A")
    readonly property color border: pick("#30333B", "#CDD1DA", "#2A2B31")

    // Interaction surfaces (hover/pressed overlays on panels)
    readonly property color surfaceHover: isLight ? Qt.rgba(0, 0, 0, 0.07) : Qt.rgba(1, 1, 1, 0.08)
    readonly property color surfacePressed: isLight ? Qt.rgba(0, 0, 0, 0.13) : Qt.rgba(1, 1, 1, 0.14)
    readonly property color focusRing: accentHover

    // Overlay scrims (controls over video fade from transparent to these).
    // Video chrome is dark in every mode, so these and the video* tokens never change.
    readonly property color scrimClear: Qt.rgba(0, 0, 0, 0)
    readonly property color scrimSoft: Qt.rgba(0, 0, 0, 0.55)
    readonly property color scrimStrong: Qt.rgba(0, 0, 0, 0.92)
    readonly property color videoBackdrop: "#000000"
    readonly property color videoText: "#F4F5F7"
    readonly property color videoTextSecondary: "#A3A7B0"
    readonly property color videoHover: Qt.rgba(1, 1, 1, 0.08)
    readonly property color videoPressed: Qt.rgba(1, 1, 1, 0.14)
    readonly property color videoTrackIdle: Qt.rgba(1, 1, 1, 0.22)

    readonly property color panelOverlay: pick(Qt.rgba(16 / 255, 17 / 255, 20 / 255, 0.95),
                                               Qt.rgba(1, 1, 1, 0.97), Qt.rgba(0, 0, 0, 0.96))
    readonly property color shadow: Qt.rgba(0, 0, 0, isLight ? 0.25 : 0.5)
    readonly property color trackIdle: isLight ? Qt.rgba(0, 0, 0, 0.18) : Qt.rgba(1, 1, 1, 0.22)

    // Text
    readonly property color primaryText: pick("#F4F5F7", "#16181D", "#F4F5F7")
    readonly property color secondaryText: pick("#A3A7B0", "#4B515C", "#A3A7B0")
    readonly property color disabledText: pick("#5A5E66", "#6A6F7A", "#6B6F78")
    readonly property color accentText: "#FFFFFF"

    // Accent
    readonly property color accent: isLight ? "#6557E8" : "#8175F5"
    readonly property color accentHover: isLight ? "#5446D6" : "#9186F7"
    readonly property color accentPressed: isLight ? "#4538BC" : "#6C60E0"
    readonly property color accentSoft: isLight ? Qt.rgba(101 / 255, 87 / 255, 232 / 255, 0.14)
                                                : Qt.rgba(129 / 255, 117 / 255, 245 / 255, 0.16)

    // Status
    readonly property color error: isLight ? "#C0392F" : "#F26B6B"
    readonly property color success: isLight ? "#1D8A52" : "#5BC98C"

    // Typography
    readonly property string fontFamily: Qt.application.font.family
    readonly property int fontSizeSmall: 11
    readonly property int fontSizeMedium: 13
    readonly property int fontSizeLarge: 15
    readonly property int fontSizeHeading: 18

    // Spacing & metrics
    readonly property int spacingSmall: 6
    readonly property int spacingMedium: 10
    readonly property int spacingLarge: 16
    readonly property int radius: 8
    readonly property int radiusSmall: 4
    readonly property int radiusLarge: 14
    readonly property string fontMono: Qt.platform.os === "osx" ? "Menlo"
                                      : Qt.platform.os === "windows" ? "Consolas" : "monospace"
    readonly property int controlHeight: 36

    // Motion (restrained). Reduced motion zeroes every Behavior/transition duration.
    property bool reducedMotion: false
    readonly property int animationFast: reducedMotion ? 0 : 120
    readonly property int animationMedium: reducedMotion ? 0 : 220
}
