import QtQuick
import Lumina.Player

// Theme-coloured line icon drawn from an SVG path on a 24x24 grid (no per-colour asset files).
Image {
    id: g

    property string name: ""
    readonly property var paths: ({
        back: "M19 12H6 M11 6L5 12L11 18",
        play: "M8 5.5L19 12L8 18.5Z",
        link: "M10 14a4 4 0 0 0 5.6 0l3-3a4 4 0 0 0-5.6-5.6l-1 1 M14 10a4 4 0 0 0-5.6 0l-3 3a4 4 0 0 0 5.6 5.6l1-1",
        library: "M4 4h7v7H4z M13 4h7v7h-7z M4 13h7v7H4z M13 13h7v7h-7z",
        folder: "M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z",
        star: "M12 3.5L14.8 9.2L21 10.1L16.5 14.5L17.6 20.7L12 17.7L6.4 20.7L7.5 14.5L3 10.1L9.2 9.2Z",
        more: "M5 12h0.01 M12 12h0.01 M19 12h0.01",
        search: "M11 4a7 7 0 1 0 0 14a7 7 0 0 0 0-14z M20 20l-4-4",
        list: "M4 6h16 M4 12h16 M4 18h16",
        check: "M5 12.5l4.5 4.5L19 7.5",
        refresh: "M20 11a8 8 0 1 0-2.3 5.7 M20 5v6h-6",
        close: "M6 6l12 12 M18 6L6 18",
        music: "M9 18V6l10-2v12 M9 18a3 3 0 1 1-6 0a3 3 0 0 1 6 0z M19 16a3 3 0 1 1-6 0a3 3 0 0 1 6 0z",
        folderPlus: "M3 7a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z M12 10.5v5 M9.5 13h5",
        settings: "M4 7h10 M18 7h2 M4 17h2 M10 17h10 M14 4v6 M6 14v6"
    })
    readonly property string d: paths[name] || ""
    property color tint: Theme.primaryText
    property bool filled: false
    property real size: 18
    property real stroke: 1.8

    readonly property string rgb: "rgb(" + Math.round(tint.r * 255) + "," + Math.round(tint.g * 255) + "," + Math.round(tint.b * 255) + ")"

    width: size
    height: size
    sourceSize: Qt.size(size * 2, size * 2)
    smooth: true
    source: "data:image/svg+xml;utf8," + encodeURIComponent(
        '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path d="' + d + '" fill="'
        + (filled ? rgb : "none") + '" fill-opacity="' + tint.a + '" stroke="' + (filled ? "none" : rgb)
        + '" stroke-opacity="' + tint.a + '" stroke-width="' + stroke + '" stroke-linecap="round" stroke-linejoin="round"/></svg>')
}
