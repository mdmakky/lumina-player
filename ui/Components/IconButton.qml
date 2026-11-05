import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Minimal icon button used across the player chrome. Icons are embedded SVGs
// colored with the theme's primary text color.
Button {
    id: control

    property string iconSource: ""
    property int iconSize: 18
    property string label: ""
    // Optional hover tooltip text.
    property string tip: ""
    // Toggle highlight (e.g. shuffle on).
    property bool active: false

    implicitWidth: iconSource !== "" ? 40 : 56
    implicitHeight: Theme.controlHeight
    flat: true
    hoverEnabled: true
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeMedium

    contentItem: Item {
        scale: control.pressed ? 0.92 : 1.0
        Behavior on scale { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }

        Image {
            visible: control.iconSource !== ""
            anchors.centerIn: parent
            source: control.iconSource
            sourceSize: Qt.size(control.iconSize, control.iconSize)
            fillMode: Image.PreserveAspectFit
            opacity: control.hovered || control.pressed ? 1.0 : 0.85
            Behavior on opacity { NumberAnimation { duration: Theme.animationFast } }
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
        radius: Theme.radius
        color: control.pressed ? Theme.surfacePressed : (control.hovered ? Theme.surfaceHover : (control.active ? Theme.accentSoft : "transparent"))
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
    }

    opacity: enabled ? 1.0 : 0.35

    // Icon-only buttons have no text; expose the tooltip/label to screen readers.
    Accessible.role: Accessible.Button
    Accessible.name: control.tip !== "" ? control.tip : control.label

    ToolTip {
        visible: control.tip !== "" && control.hovered
        delay: 500
        text: control.tip
        contentItem: Text {
            text: control.tip
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }
        background: Rectangle {
            color: Theme.elevated
            border.color: Theme.border
            radius: Theme.radiusSmall
        }
    }
}
