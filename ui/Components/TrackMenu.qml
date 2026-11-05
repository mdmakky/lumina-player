import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs

// Audio / subtitle track picker plus sync delay controls. Themed Menu.
Menu {
    id: menu

    required property var engine

    function trackLabel(t) {
        var s = t.title || t.lang || qsTr("Track %1").arg(t.id)
        if (t.title && t.lang) s += " [" + t.lang + "]"
        return s
    }
    function ms(v) { return Math.round(v * 1000) + " ms" }

    background: Rectangle {
        implicitWidth: 280
        color: Theme.elevated
        border.color: Theme.border
        radius: Theme.radius
    }

    component Entry: MenuItem {
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
        implicitHeight: Theme.controlHeight
        indicator: Rectangle {
            visible: parent.checked
            x: 10
            y: (parent.height - height) / 2
            width: 6
            height: 6
            radius: 3
            color: Theme.accent
        }
        contentItem: Text {
            text: parent.text
            font: parent.font
            color: parent.checked ? Theme.accentHover : Theme.primaryText
            verticalAlignment: Text.AlignVCenter
            leftPadding: 24
        }
        background: Rectangle {
            color: parent.highlighted ? Theme.accentSoft : "transparent"
            radius: Theme.radius
        }
    }

    component Heading: MenuItem {
        enabled: false
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeSmall
        contentItem: Text { text: parent.text; font: parent.font; color: Theme.secondaryText; leftPadding: 24 }
        background: null
    }

    Heading { text: qsTr("AUDIO") }
    Repeater {
        model: menu.engine.audioTracks
        delegate: Entry {
            required property var modelData
            objectName: "audioItem_" + modelData.id
            text: menu.trackLabel(modelData)
            checkable: true
            checked: modelData.id === menu.engine.audioTrackId
            onTriggered: menu.engine.setAudioTrack(modelData.id)
        }
    }

    MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

    Heading { text: qsTr("SUBTITLES") }
    Entry {
        objectName: "subItem_off"
        text: qsTr("Off")
        checkable: true
        checked: menu.engine.subtitleTrackId === 0
        onTriggered: menu.engine.setSubtitleTrack(0)
    }
    Repeater {
        model: menu.engine.subtitleTracks
        delegate: Entry {
            required property var modelData
            objectName: "subItem_" + modelData.id
            text: menu.trackLabel(modelData)
            checkable: true
            checked: modelData.id === menu.engine.subtitleTrackId
            onTriggered: menu.engine.setSubtitleTrack(modelData.id)
        }
    }
    Entry { text: qsTr("Load external file…"); onTriggered: subDialog.open() }

    MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.border } }

    Heading { text: qsTr("SYNC   audio %1 · subs %2").arg(menu.ms(menu.engine.audioDelay)).arg(menu.ms(menu.engine.subtitleDelay)) }
    Entry { text: qsTr("Audio earlier (−100 ms)"); onTriggered: menu.engine.setAudioDelay(menu.engine.audioDelay - 0.1) }
    Entry { text: qsTr("Audio later (+100 ms)"); onTriggered: menu.engine.setAudioDelay(menu.engine.audioDelay + 0.1) }
    Entry { text: qsTr("Subtitles earlier (−100 ms, G)"); onTriggered: menu.engine.setSubtitleDelay(menu.engine.subtitleDelay - 0.1) }
    Entry { text: qsTr("Subtitles later (+100 ms, H)"); onTriggered: menu.engine.setSubtitleDelay(menu.engine.subtitleDelay + 0.1) }

    FileDialog {
        id: subDialog
        title: qsTr("Load Subtitle")
        nameFilters: [qsTr("Subtitles (*.srt *.ass *.ssa *.vtt *.sub)"), qsTr("All files (*)")]
        onAccepted: menu.engine.loadExternalSubtitle(selectedFile)
    }
}
