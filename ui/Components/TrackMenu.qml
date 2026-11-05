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
        id: subDialog
        title: qsTr("Load Subtitle")
        nameFilters: [qsTr("Subtitles (*.srt *.ass *.ssa *.vtt *.sub)"), qsTr("All files (*)")]
        onAccepted: menu.engine.loadExternalSubtitle(selectedFile)
    }
}
