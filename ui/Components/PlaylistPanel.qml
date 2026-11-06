import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Lumina.Player

// Right-side play queue. Talks to the controller only (player.playlist model,
// player.playIndex). Rows reorder by dragging the grip; disabled while a
// search filter is active (visible rows no longer map 1:1 to queue positions).
Rectangle {
    id: panel

    width: Math.min(360, parent ? parent.width - 2 * Theme.spacingMedium : 360)
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
                text: qsTr("Queue · %1").arg(player.playlist.count)
                elide: Text.ElideRight
                Layout.minimumWidth: 0
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                font.weight: Font.DemiBold
                Layout.fillWidth: true
            }
            IconButton {
                onPanel: true
                objectName: "shuffleButton"
                iconSource: "qrc:/qt/qml/Lumina/Player/icons/shuffle.svg"
                implicitWidth: 32
                active: player.shuffle
                tip: qsTr("Shuffle (Shift+S)")
                onClicked: player.shuffle = !player.shuffle
            }
            IconButton {
                onPanel: true
                objectName: "repeatButton"
                iconSource: "qrc:/qt/qml/Lumina/Player/icons/" + (player.repeatMode === 2 ? "repeat-one.svg" : "repeat.svg")
                implicitWidth: 32
                active: player.repeatMode !== 0
                tip: qsTr("Repeat: off / all / one (R)")
                onClicked: player.repeatMode = (player.repeatMode + 1) % 3
            }
            IconButton {
                onPanel: true
                objectName: "playlistSaveButton"
                label: qsTr("Save")
                implicitWidth: 46
                onClicked: saveDialog.open()
            }
            IconButton {
                onPanel: true
                objectName: "playlistLoadButton"
                label: qsTr("Load")
                implicitWidth: 46
                onClicked: loadDialog.open()
            }
            IconButton {
                onPanel: true
                objectName: "playlistClearButton"
                label: qsTr("Clear")
                implicitWidth: 46
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
            property int dragFrom: -1
            property int dropTarget: -1

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

                property real dragY: 0
                transform: Translate { y: row.dragY }
                z: grip.active ? 10 : 0
                HoverHandler { id: hover }
                TapHandler {
                    onDoubleTapped: player.playIndex(row.index)
                }

                Rectangle { // drop indicator
                    visible: list.dropTarget === row.index && list.dragFrom !== row.index
                    width: parent.width
                    height: 2
                    color: Theme.accent
                    y: list.dragFrom > row.index ? 0 : parent.height - 2
                    z: 20
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMedium
                    anchors.rightMargin: Theme.spacingSmall
                    spacing: 2

                    Text { // drag grip
                        objectName: "rowGrip_" + row.index
                        text: "⋮⋮"
                        visible: !panel.filtering
                        color: Theme.secondaryText
                        font.pixelSize: Theme.fontSizeMedium
                        Layout.rightMargin: Theme.spacingSmall
                        Layout.preferredHeight: row.height
                        verticalAlignment: Text.AlignVCenter
                        HoverHandler { cursorShape: Qt.OpenHandCursor }
                        DragHandler {
                            id: grip
                            target: null
                            xAxis.enabled: false
                            enabled: !panel.filtering
                            onTranslationChanged: {
                                if (!active) return
                                row.dragY = translation.y
                                list.dragFrom = row.index
                                list.dropTarget = Math.max(0, Math.min(list.count - 1,
                                    row.index + Math.round(translation.y / row.height)))
                            }
                            onActiveChanged: {
                                if (active) return
                                const to = list.dropTarget
                                row.dragY = 0
                                list.dragFrom = -1
                                list.dropTarget = -1
                                if (to >= 0 && to !== row.index)
                                    player.playlist.move(row.index, to)
                            }
                        }
                    }
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
                        onPanel: true
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
