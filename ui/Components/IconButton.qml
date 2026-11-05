import QtQuick
import QtQuick.Controls.Basic

// Minimal icon button used across the player chrome. Icons are embedded SVGs
// colored with the theme's primary text color.
Button {
    id: control

    property string iconSource: ""
    property int iconSize: 18
    property string label: ""

    implicitWidth: iconSource !== "" ? 40 : 56
    implicitHeight: Theme.controlHeight
    flat: true
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeMedium

    contentItem: Item {
        Image {
            visible: control.iconSource !== ""
            anchors.centerIn: parent
            source: control.iconSource
            sourceSize: Qt.size(control.iconSize, control.iconSize)
            fillMode: Image.PreserveAspectFit
        }
        Text {
            visible: control.label !== ""
            anchors.centerIn: parent
            text: control.label
            color: control.pressed ? Theme.accentPressed : (control.hovered ? Theme.accentHover : Theme.accent)
            font: control.font
        }
    }

    background: Rectangle {
        color: control.pressed ? Theme.elevated : (control.hovered ? Qt.lighter(Theme.surface, 1.35) : "transparent")
        radius: Theme.radius
    }

    opacity: enabled ? 1.0 : 0.35
}
