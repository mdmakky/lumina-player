import QtQuick
import QtQuick.Controls.Basic

// Seek timeline. While dragging, shows a preview position and only seeks once
// released — prevents seek flooding and keeps mpv responsive.
Slider {
    id: control

    property double preview: 0
    readonly property bool dragging: pressed

    from: 0
    to: player.duration > 0 ? player.duration : 1
    value: dragging ? preview : player.position
    onMoved: preview = value

    onPressedChanged: if (!pressed) player.seek(preview)

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: 22

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: 4
        radius: 2
        color: Theme.elevated

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: 2
            color: Theme.accent
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.hovered || control.pressed ? 14 : 10
        height: width
        radius: width / 2
        color: Theme.primaryText
        Behavior on width { NumberAnimation { duration: Theme.animationFast } }
    }

    ToolTip {
        parent: control.handle
        visible: control.hovered || control.dragging
        y: -height - 6
        background: Rectangle {
            color: Theme.elevated
            border.color: Theme.border
            radius: Theme.radius
        }
        contentItem: Text {
            text: timefmt.formatDuration(control.dragging ? control.preview : control.value)
            color: Theme.primaryText
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
