import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import Lumina.Player

// Audio / subtitle track picker plus sync delay controls. Track labels come from the
// engine (label = language name, detail = codec · layout · tags; unique per list).
PopupPanel {
    id: menu

    required property var engine

    panelWidth: 320

    MenuSectionHeader { text: qsTr("AUDIO") }
    Repeater {
        model: menu.engine.audioTracks
        delegate: MenuRow {
            required property var modelData
            objectName: "audioItem_" + modelData.id
            text: modelData.label
            secondary: modelData.detail
            mark: "radio"
            selected: modelData.id === menu.engine.audioTrackId
            onTriggered: menu.engine.setAudioTrack(modelData.id)
        }
    }

    MenuRow {
        objectName: "audioLoadExternal"
        text: qsTr("Load external audio…")
        iconSource: "qrc:/qt/qml/Lumina/Player/icons/folder-open.svg"
        onTriggered: audioDialog.open()
    }
    MenuRow {
        // Cycles Off -> Spectrum -> Waveform for audio-only files (the full list lives in Settings > Audio).
        objectName: "vizCycle"
        readonly property var modes: ["off", "spectrum", "waveform", "scope", "freqs"]
        visible: menu.engine.audioTracks.length > 0 && menu.engine.videoTracks.every(t => t.albumart || t.image)
        text: qsTr("Visualization: %1").arg(menu.engine.visualization === "off" ? qsTr("Off")
              : menu.engine.visualization === "spectrum" ? qsTr("Spectrum")
              : menu.engine.visualization === "waveform" ? qsTr("Waveform")
              : menu.engine.visualization === "scope" ? qsTr("Vectorscope") : qsTr("Frequency bars"))
        closeOnClick: false
        onTriggered: {
            const m = modes[(modes.indexOf(menu.engine.visualization) + 1) % modes.length]
            menu.engine.setVisualization(m)
            player.setSetting("audio/visualization", m)
        }
    }

    PanelSeparator {}

    MenuSectionHeader { text: qsTr("SUBTITLES") }
    MenuRow {
        objectName: "subItem_off"
        text: qsTr("Off")
        mark: "radio"
        selected: menu.engine.subtitleTrackId === 0
        onTriggered: menu.engine.setSubtitleTrack(0)
    }
    Repeater {
        model: menu.engine.subtitleTracks
        delegate: MenuRow {
            required property var modelData
            objectName: "subItem_" + modelData.id
            text: modelData.label
            secondary: modelData.detail
            mark: "radio"
            selected: modelData.id === menu.engine.subtitleTrackId
            onTriggered: menu.engine.setSubtitleTrack(modelData.id)
        }
    }
    MenuRow {
        objectName: "subLoadExternal"
        text: qsTr("Load external file…")
        iconSource: "qrc:/qt/qml/Lumina/Player/icons/folder-open.svg"
        onTriggered: subDialog.open()
    }

    PanelSeparator {}

    MenuSectionHeader {
        text: qsTr("Sync")
        actionText: qsTr("Reset")
        actionObjectName: "syncReset"
        actionEnabled: menu.engine.audioDelay !== 0 || menu.engine.subtitleDelay !== 0
        onActionClicked: { menu.engine.setAudioDelay(0); menu.engine.setSubtitleDelay(0) }
    }
    DelayStepper {
        objectName: "audioDelay"
        label: qsTr("Audio delay")
        seconds: menu.engine.audioDelay
        onStepped: (d) => menu.engine.setAudioDelay(menu.engine.audioDelay + d)
    }
    DelayStepper {
        objectName: "subDelay"
        label: qsTr("Subtitle delay")
        seconds: menu.engine.subtitleDelay
        onStepped: (d) => menu.engine.setSubtitleDelay(menu.engine.subtitleDelay + d)
    }

    FileDialog {
        id: audioDialog
        title: qsTr("Load Audio")
        nameFilters: [qsTr("Audio (*.mp3 *.flac *.m4a *.aac *.ogg *.opus *.wav *.mka *.ac3 *.dts)"), qsTr("All files (*)")]
        onAccepted: menu.engine.loadExternalAudio(selectedFile)
    }
    FileDialog {
        id: subDialog
        title: qsTr("Load Subtitle")
        nameFilters: [qsTr("Subtitles (*.srt *.ass *.ssa *.vtt *.sub)"), qsTr("All files (*)")]
        onAccepted: menu.engine.loadExternalSubtitle(selectedFile)
    }
}
