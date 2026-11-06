import QtQuick
import Lumina.Player

// Output device, channel layout, volume normalization, passthrough and visualization.
// Each choice is applied live and persisted under audio/*.
SettingSection {
    id: sec
    title: qsTr("Output and processing")

    function pick(key, v, apply) { apply(v); player.setSetting(key, v) }

    AudioDeviceRow {}
    ComboRow {
        name: "audioChannels"
        title: qsTr("Channels")
        description: qsTr("Auto keeps the file's layout. Stereo downmix suits headphones and laptop speakers.")
        keywords: "stereo mono surround 5.1 7.1 downmix upmix"
        model: [
            { label: qsTr("Auto"), value: "auto" },
            { label: qsTr("Stereo downmix"), value: "stereo" },
            { label: qsTr("Mono"), value: "mono" },
            { label: qsTr("5.1 surround"), value: "5.1" },
            { label: qsTr("7.1 surround"), value: "7.1" }
        ]
        value: { SettingsState.resetTick; return player.setting("audio/channels", "auto") }
        onPicked: (v) => { sec.pick("audio/channels", v, player.engine.setAudioChannels); SettingsState.resetTick++ }
    }
    ComboRow {
        name: "audioNormalization"
        title: qsTr("Volume normalization")
        description: qsTr("Night mode strongly evens out loud and quiet parts. Movie targets a steady loudness.")
        keywords: "night mode compressor dynamic range loudness dynaudnorm loudnorm"
        model: [
            { label: qsTr("Off"), value: "off" },
            { label: qsTr("Night mode"), value: "night" },
            { label: qsTr("Movie"), value: "movie" }
        ]
        value: { SettingsState.resetTick; return player.setting("audio/normalization", "off") }
        onPicked: (v) => { sec.pick("audio/normalization", v, player.engine.setNormalization); SettingsState.resetTick++ }
    }
    SwitchRow {
        name: "audioPassthrough"
        title: qsTr("Audio passthrough")
        description: qsTr("Sends AC3, E-AC3, DTS and TrueHD untouched over HDMI or S/PDIF. Needs a receiver that decodes them; the equalizer and normalization are skipped for those tracks.")
        keywords: "spdif hdmi dolby dts truehd receiver bitstream"
        checked: { SettingsState.resetTick; const v = player.setting("audio/passthrough", false); return v === true || v === "true" }
        onToggled: (v) => { sec.pick("audio/passthrough", v, player.engine.setAudioPassthrough); SettingsState.resetTick++ }
    }
    ComboRow {
        name: "audioVisualization"
        title: qsTr("Audio visualization")
        description: qsTr("Draws the music for audio-only files that have no cover art or video.")
        keywords: "spectrum waveform vectorscope visualizer music"
        model: [
            { label: qsTr("Off"), value: "off" },
            { label: qsTr("Spectrum"), value: "spectrum" },
            { label: qsTr("Waveform"), value: "waveform" },
            { label: qsTr("Vectorscope"), value: "scope" },
            { label: qsTr("Frequency bars"), value: "freqs" }
        ]
        value: { SettingsState.resetTick; return player.setting("audio/visualization", "off") }
        onPicked: (v) => { sec.pick("audio/visualization", v, player.engine.setVisualization); SettingsState.resetTick++ }
    }
}
