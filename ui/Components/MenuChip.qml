import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Compact pill for short option sets (aspect ratio, rotation) inside a PopupPanel.
AbstractButton {
    id: chip

    property bool selected: false
    property bool closeOnClick: true
    signal triggered()

    implicitHeight: 28
    implicitWidth: Math.max(48, label.implicitWidth + 24)
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: Accessible.RadioButton
    Accessible.checked: selected

    function closePanel() {
        for (let p = chip.parent; p; p = p.parent) {
            if (p.panelRef !== undefined) { p.panelRef.close(); return }
        }
    }
    onClicked: { triggered(); if (closeOnClick) closePanel() }
    Keys.onReturnPressed: click()
    Keys.onEnterPressed: click()

    contentItem: Text {
        id: label
        text: chip.text
        color: chip.selected ? Theme.accentHover : Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: height / 2
        color: chip.selected ? Theme.accentSoft : (chip.down ? Theme.surfacePressed : (chip.hovered ? Theme.surfaceHover : "transparent"))
        border.width: 1
        border.color: chip.visualFocus ? Theme.focusRing : (chip.selected ? Theme.accent : Theme.border)
    }
}
