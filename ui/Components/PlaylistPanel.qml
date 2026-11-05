import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Lumina.Player

// Right-side play queue. Talks to the controller only (player.playlist model,
// player.playIndex). Reorder uses up/down buttons and is disabled while a
// search filter is active (visible rows no longer map 1:1 to queue positions).
Rectangle {
    id: panel

    width: 320
    color: Theme.surface
    border.color: Theme.border

    readonly property bool filtering: search.text !== ""

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMedium
        spacing: Theme.spacingSmall

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Playlist (%1)").arg(player.playlist.count)
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            IconButton {
                objectName: "playlistSaveButton"
                label: qsTr("Save")
                onClicked: saveDialog.open()
            }
            IconButton {
                objectName: "playlistLoadButton"
                label: qsTr("Load")
                onClicked: loadDialog.open()
            }
            IconButton {
                objectName: "playlistClearButton"
                label: qsTr("Clear")
                onClicked: player.playlist.clear()
            }
        }

        TextField {
            id: search
            objectName: "playlistSearch"
            Layout.fillWidth: true
            placeholderText: qsTr("Search")
            color: Theme.primaryText
            placeholderTextColor: Theme.disabledText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            background: Rectangle {
                color: Theme.elevated
                radius: Theme.radius
                border.color: search.activeFocus ? Theme.accent : Theme.border
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: player.playlist
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                id: row
                objectName: "playlistRow_" + index
                required property int index
                required property string title
                required property string path
                required property bool current

                readonly property bool matches: !panel.filtering
                    || title.toLowerCase().indexOf(search.text.toLowerCase()) >= 0

                width: ListView.view.width
                height: matches ? 36 : 0
                visible: matches
                radius: Theme.radius
                color: current ? Theme.accentSoft : (hover.hovered ? Theme.elevated : "transparent")

                HoverHandler { id: hover }
                TapHandler {
                    onDoubleTapped: player.playIndex(row.index)
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMedium
                    anchors.rightMargin: Theme.spacingSmall
                    spacing: 2

                    Text {
                        Layout.fillWidth: true
                        text: row.title
                        elide: Text.ElideMiddle
                        color: row.current ? Theme.accent : Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                        font.weight: row.current ? Font.DemiBold : Font.Normal
                    }
                    IconButton {
                        objectName: "rowUp_" + row.index
                        implicitWidth: 28
                        label: "▲"
                        enabled: !panel.filtering && row.index > 0
                        onClicked: player.playlist.move(row.index, row.index - 1)
                    }
                    IconButton {
                        objectName: "rowDown_" + row.index
                        implicitWidth: 28
                        label: "▼"
                        enabled: !panel.filtering && row.index < player.playlist.count - 1
                        onClicked: player.playlist.move(row.index, row.index + 1)
                    }
                    IconButton {
                        objectName: "rowRemove_" + row.index
                        implicitWidth: 28
                        label: "✕"
                        onClicked: player.playlist.remove(row.index)
                    }
                }
            }
        }

        Text {
            visible: player.playlist.count === 0
            Layout.fillWidth: true
            text: qsTr("Queue is empty. Drop files or press Ctrl+O.")
            wrapMode: Text.WordWrap
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save Playlist")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Playlists (*.m3u *.m3u8)")]
        defaultSuffix: "m3u8"
        onAccepted: player.savePlaylist(selectedFile)
    }
    FileDialog {
        id: loadDialog
        title: qsTr("Load Playlist")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Playlists (*.m3u *.m3u8)")]
        onAccepted: player.loadPlaylist(selectedFile)
    }
}
