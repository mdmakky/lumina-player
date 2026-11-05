import QtQuick
import QtQuick.Controls.Basic

// Volume control (0-150%). Drop-in replacement for the inline Slider in Main.qml.
Slider {
    id: control

    from: 0
    to: 150
    value: player.engine.volume
    onMoved: player.engine.setVolume(value)

    hoverEnabled: true
    implicitWidth: 90
    implicitHeight: 20

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.hovered || control.pressed ? 5 : 3
        radius: height / 2
        color: Theme.videoTrackIdle
        Behavior on height { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: Theme.accent
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.hovered || control.pressed ? 12 : 8
        height: width
        radius: width / 2
        color: Theme.videoText
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Behavior on width { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
    }
}
