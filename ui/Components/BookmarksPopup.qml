import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Bookmarks of the current file: click a row to seek, x to delete.
PopupPanel {
    id: popup

    focus: true
    padding: Theme.spacingLarge
    panelWidth: 300

    contentItem: ColumnLayout {
        spacing: Theme.spacingSmall
        Text {
            text: qsTr("Bookmarks")
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
        }
        Text {
            visible: player.bookmarks.length === 0
            text: qsTr("No bookmarks. Press B to add one.")
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
        }
        Repeater {
            model: player.bookmarks
            delegate: Rectangle {
                required property var modelData
                required property int index
                objectName: "bookmarkRow_" + index
                Layout.fillWidth: true
                implicitHeight: 32
                radius: Theme.radiusSmall
                color: rowArea.containsMouse ? Theme.surfaceHover : "transparent"
                MouseArea {
                    id: rowArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: player.engine.seek(modelData.time)
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingMedium
                    Text {
                        text: timefmt.formatDuration(modelData.time) + (modelData.label ? "   " + modelData.label : "")
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    ToolButton {
                        objectName: "bookmarkDelete_" + index
                        text: "✕"
                        onClicked: player.removeBookmark(modelData.id)
                        contentItem: Text {
                            text: parent.text
                            color: Theme.secondaryText
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        background: null
                    }
                }
            }
        }
    }
}
