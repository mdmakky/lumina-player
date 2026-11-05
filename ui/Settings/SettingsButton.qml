import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Themed push button for the settings dialog (primary, default or danger look).
Button {
    id: btn

    property bool primary: false
    property bool danger: false

    implicitHeight: 32
    implicitWidth: Math.max(34, label.implicitWidth + 28)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1 : 0.45

    Accessible.role: Accessible.Button
    Accessible.name: btn.text

    background: Rectangle {
        radius: Theme.radius
        color: btn.primary ? (btn.pressed ? Theme.accentPressed : btn.hovered ? Theme.accentHover : Theme.accent)
             : btn.pressed ? Theme.surfacePressed
             : btn.hovered ? Theme.surfaceHover : Theme.elevated
        border.width: btn.visualFocus ? 2 : (btn.primary ? 0 : 1)
        border.color: btn.visualFocus ? Theme.focusRing : (btn.danger ? Theme.error : Theme.border)
    }
    contentItem: Text {
        id: label
        text: btn.text
        color: btn.primary ? Theme.accentText : btn.danger ? Theme.error : Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
