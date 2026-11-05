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
    // True when drawn on a themed panel (not over the dark video chrome): uses dark icons in Light mode.
    property bool onPanel: false

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
            source: control.onPanel && Theme.isLight ? control.iconSource.replace(".svg", "-dark.svg") : control.iconSource
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
        color: control.pressed ? (control.onPanel ? Theme.surfacePressed : Theme.videoPressed) : (control.hovered ? (control.onPanel ? Theme.surfaceHover : Theme.videoHover) : (control.active ? Theme.accentSoft : "transparent"))
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
    }

    opacity: enabled ? 1.0 : 0.35

    // Icon-only buttons have no text; expose the tooltip/label to screen readers.
    Accessible.role: Accessible.Button
    Accessible.name: control.tip !== "" ? control.tip : control.label

    ThemedToolTip {
        visible: control.tip !== "" && control.hovered
        text: control.tip
    }
}
