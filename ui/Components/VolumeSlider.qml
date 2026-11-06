import QtQuick
import QtQuick.Controls.Basic

// Volume control with a live percentage. Follows the OS output volume when the platform reports it
// (player.systemVolume >= 0, range 0-100); otherwise drives the engine volume (0-150).
Slider {
    id: control

    readonly property bool system: player.systemVolume >= 0

    from: 0
    to: system ? 100 : 150
    value: player.engine.muted ? 0 : (system ? player.systemVolume : player.engine.volume)
    onMoved: player.setVolumePercent(Math.round(value))

    hoverEnabled: true
    stepSize: 1
    implicitWidth: 128
    implicitHeight: 24

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.hovered || control.pressed ? 6 : 4
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
        width: control.hovered || control.pressed ? 14 : 0
        height: width
        radius: width / 2
        color: Theme.videoText
        border.width: control.visualFocus ? 2 : 0
        border.color: Theme.focusRing
        Behavior on width { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }

        // Percentage bubble above the handle while hovering or dragging.
        Rectangle {
            visible: control.hovered || control.pressed
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.top
            anchors.bottomMargin: 6
            width: pct.implicitWidth + 12
            height: pct.implicitHeight + 6
            radius: 6
            color: Qt.rgba(0, 0, 0, 0.72)
            Text {
                id: pct
                anchors.centerIn: parent
                text: Math.round(control.value) + "%"
                color: Theme.videoText
                font.pixelSize: Theme.fontSizeSmall
                font.weight: Font.Medium
            }
        }
    }

    // Always-visible value next to the bar (fixed width so the layout never jumps).
    Text {
        anchors.left: parent.right
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        width: 34
        text: Math.round(control.value) + "%"
        color: Theme.videoTextSecondary
        font.pixelSize: Theme.fontSizeSmall
    }
}
