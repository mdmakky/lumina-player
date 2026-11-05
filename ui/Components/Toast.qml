import QtQuick
import Lumina.Player

// Transient message pill. Call show(text) from anywhere.
Rectangle {
    id: toast

    function show(message) {
        label.text = message
        opacity = 1
        hideTimer.restart()
    }

    implicitWidth: label.implicitWidth + 2 * Theme.spacingLarge
    implicitHeight: label.implicitHeight + 2 * Theme.spacingMedium
    radius: height / 2
    color: Theme.panelOverlay
    border.color: Theme.border
    opacity: 0
    visible: opacity > 0
    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }

    Text {
        id: label
        anchors.centerIn: parent
        color: Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
    }
    Timer { id: hideTimer; interval: 2200; onTriggered: toast.opacity = 0 }
}
