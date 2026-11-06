import QtQuick
import Lumina.Player

// Audio output device picker. The list is re-read from mpv whenever the row becomes visible,
// so devices plugged in meanwhile (USB, Bluetooth, HDMI) show up.
ComboRow {
    id: row

    title: qsTr("Output device")
    description: qsTr("Where Lumina plays sound. Automatic follows the system default.")
    keywords: "speakers headphones hdmi bluetooth usb sound card pipewire"
    name: "audioDevice"
    value: player.setting("audio/device", "auto")

    function refresh() { model = player.engine.audioDevices() }
    onVisibleChanged: if (visible) refresh()
    Component.onCompleted: refresh()

    onPicked: (v) => {
        player.engine.setAudioDevice(v)
        player.setSetting("audio/device", v)
        value = v
    }
}
