import QtQuick
import Lumina.Player

// Settings > Video: default scaling, HDR tone mapping and frame interpolation.
// Same keys as the Picture popup (video/scale, video/tonemap, video/interp); applied live and at startup.
SettingSection {
    title: qsTr("Quality")

    function pick(key, v) {
        player.engine.setPicture(key, v)
        player.setSetting("video/" + key, v)
    }

    ComboRow {
        name: "videoScale"
        title: qsTr("Scaling quality")
        description: qsTr("High quality uses a sharper scaler and needs a capable GPU.")
        keywords: "scale upscale lanczos fast balanced quality"
        model: [
            { label: qsTr("Fast"), value: "fast" }, { label: qsTr("Balanced"), value: "balanced" },
            { label: qsTr("High quality"), value: "high" }
        ]
        value: player.engine.picture.scale
        onPicked: (v) => pick("scale", v)
    }
    ComboRow {
        name: "videoTonemap"
        title: qsTr("HDR tone mapping")
        description: qsTr("How HDR video is squeezed onto a standard display.")
        keywords: "hdr tone mapping bt.2390 reinhard hable"
        model: [
            { label: qsTr("Auto"), value: "auto" }, { label: "BT.2390", value: "bt.2390" },
            { label: "Reinhard", value: "reinhard" }, { label: "Hable", value: "hable" }
        ]
        value: player.engine.picture.tonemap
        onPicked: (v) => pick("tonemap", v)
    }
    SwitchRow {
        name: "videoInterp"
        title: qsTr("Smooth motion (frame interpolation)")
        description: qsTr("Blends frames to match your display refresh. Uses more GPU; off by default.")
        keywords: "interpolation smooth motion fps display resample"
        checked: player.engine.picture.interp
        onToggled: (v) => pick("interp", v)
    }
}
