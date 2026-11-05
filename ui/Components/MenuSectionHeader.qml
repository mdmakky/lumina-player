import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Small caps section title, with an optional right-aligned text action.
Item {
    id: header

    property string text: ""
    property string actionText: ""
    property bool actionEnabled: true
    property string actionObjectName: ""
    signal actionClicked()

    Layout.fillWidth: true
    implicitHeight: 28

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        text: header.text.toUpperCase()
        color: Theme.secondaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeSmall
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
    }
    AbstractButton {
        objectName: header.actionObjectName
        visible: header.actionText !== ""
        enabled: header.actionEnabled
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 2
        implicitHeight: 22
        implicitWidth: actionLabel.implicitWidth + 16
        hoverEnabled: true
        text: header.actionText
        onClicked: header.actionClicked()
        opacity: enabled ? 1 : 0.4
        contentItem: Text {
            id: actionLabel
            text: parent.text
            color: parent.hovered ? Theme.accentHover : Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radiusSmall
            color: parent.down ? Theme.surfacePressed : (parent.hovered ? Theme.surfaceHover : "transparent")
            border.width: parent.visualFocus ? 1 : 0
            border.color: Theme.focusRing
        }
    }
}
