import QtQuick
import Lumina.Player

// Compact playback statistics panel. Toggle via engine.statsEnabled.
Rectangle {
    id: root
    required property var engine
    readonly property var s: engine.stats

    function kbps(v) { return v ? Math.round(v / 1000) + " kb/s" : "-" }
    function num(v, d) { return (v === undefined || v === null || v === "") ? "-" : Number(v).toFixed(d) }

    color: Theme.panelOverlay
    radius: Theme.radiusSmall
    implicitWidth: label.implicitWidth + 2 * Theme.spacingMedium
    implicitHeight: label.implicitHeight + 2 * Theme.spacingMedium

    Text {
        id: label
        x: Theme.spacingMedium; y: Theme.spacingMedium
        color: Theme.primaryText
        font.family: Theme.fontMono
        font.pixelSize: Theme.fontSizeSmall
        text: "hwdec   " + (root.s.hwdec || "-")
            + "\ncodec   " + (root.s.codec || "-") + " (" + (root.s.format || "-") + ")"
            + "\nsize    " + (root.s.width || "-") + "x" + (root.s.height || "-")
            + "\nfps     " + root.num(root.s.fps, 1) + " / " + root.num(root.s.containerFps, 1)
            + "\ndrops   " + (root.s.droppedFrames ?? "-") + " (dec " + (root.s.decoderDroppedFrames ?? "-") + ")"
            + "\nvideo   " + root.kbps(root.s.videoBitrate)
            + "\naudio   " + root.kbps(root.s.audioBitrate)
            + "\navsync  " + root.num(root.s.avsync, 3) + " s"
            + "\ncache   " + root.num(root.s.cacheDuration, 1) + " s, " + root.kbps((root.s.cacheSpeed || 0) * 8)
    }
}
