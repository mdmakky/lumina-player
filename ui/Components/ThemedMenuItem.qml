import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Menu entry styled with theme tokens (the Basic style is light by default).
MenuItem {
    id: control

    implicitHeight: 34
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeMedium

    contentItem: Text {
        text: control.text
        font: control.font
        color: control.checked ? Theme.accentHover : Theme.primaryText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        leftPadding: 10
        rightPadding: 10
    }

    indicator: Item {}

    background: Rectangle {
        radius: Theme.radius - 2
        color: control.highlighted ? Theme.surfaceHover : "transparent"
    }
}
