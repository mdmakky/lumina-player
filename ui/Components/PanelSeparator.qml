import QtQuick
import QtQuick.Layouts
import Lumina.Player

// Hairline divider between panel sections.
Item {
    Layout.fillWidth: true
    implicitHeight: 9
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 6
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        height: 1
        color: Theme.border
    }
}
