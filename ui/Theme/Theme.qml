pragma Singleton
import QtQuick

// Centralized design tokens (spec §9). Components must never hardcode colors —
// reference the theme instead. Dark-first, cinematic palette.
QtObject {
    // Surfaces
    readonly property color background: "#101114"
    readonly property color surface: "#191B20"
    readonly property color elevated: "#22252B"
    readonly property color border: "#30333B"

    // Text
    readonly property color primaryText: "#F4F5F7"
    readonly property color secondaryText: "#A3A7B0"
    readonly property color disabledText: "#5A5E66"

    // Accent
    readonly property color accent: "#8175F5"
    readonly property color accentHover: "#9186F7"
    readonly property color accentPressed: "#6C60E0"
    readonly property color accentSoft: Qt.rgba(129 / 255, 117 / 255, 245 / 255, 0.16)

    // Status
    readonly property color error: "#F26B6B"
    readonly property color success: "#5BC98C"

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
    readonly property int controlHeight: 36

    // Motion (restrained; respects nothing special for now — reduced-motion
    // support lands with Milestone 3 polish)
    readonly property int animationFast: 120
    readonly property int animationMedium: 220
}
