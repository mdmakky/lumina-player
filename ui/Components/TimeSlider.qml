import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Seek timeline. While dragging, shows a preview position and only seeks once
// released — prevents seek flooding and keeps mpv responsive.
Slider {
    id: control

    property double preview: 0
    readonly property bool dragging: pressed
    readonly property bool active: hovered || pressed

    from: 0
    to: player.engine.duration > 0 ? player.engine.duration : 1
    value: dragging ? preview : player.engine.position
    onMoved: preview = value

    onPressedChanged: if (!pressed) player.engine.seek(preview)

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: 22

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.active ? 6 : 3
        radius: height / 2
        color: Theme.trackIdle
        Behavior on height { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }

        Rectangle {
            width: control.visualPosition * parent.width
            height: parent.height
            radius: parent.radius
            color: Theme.accent
        }

        // A-B loop range and endpoint markers
        Rectangle {
            readonly property double a: player.engine.loopA
            readonly property double b: player.engine.loopB
            visible: a >= 0 && control.to > 0
            x: a / control.to * parent.width
            width: b > a ? (b - a) / control.to * parent.width : 2
            height: parent.height
            radius: 1
            color: Theme.success
            opacity: 0.85
        }
        Repeater {
            model: [player.engine.loopA, player.engine.loopB]
            delegate: Rectangle {
                required property double modelData
                required property int index
                visible: modelData >= 0 && control.to > 0
                x: modelData / control.to * parent.width - 1
                y: -4
                width: 3
                height: parent.height + 8
                radius: 1
                color: Theme.success
            }
        }

        // Bookmark markers
        Repeater {
            model: player.engine.duration > 0 ? player.bookmarks : []
            delegate: Rectangle {
                required property var modelData
                objectName: "bookmarkMarker"
                x: modelData.time / control.to * parent.width - 3
                y: parent.height / 2 - 3
                width: 6
                height: 6
                radius: 3
                color: Theme.primaryText
            }
        }

        // Chapter markers
        Repeater {
            model: player.engine.duration > 0 ? player.engine.chapters : []
            delegate: Rectangle {
                required property var modelData
                visible: modelData.time > 0
                x: modelData.time / control.to * parent.width - 1
                y: -2
                width: 2
                height: parent.height + 4
                color: Theme.secondaryText
            }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.active ? 14 : 0
        height: width
        radius: width / 2
        color: Theme.primaryText
        border.width: 2
        border.color: Theme.accent
        Behavior on width { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
    }

    ToolTip {
        parent: control.handle
        visible: control.hovered || control.dragging
        y: -height - 8
        background: Rectangle {
            color: Theme.elevated
            border.color: Theme.border
            radius: Theme.radiusSmall
        }
        contentItem: Text {
            text: timefmt.formatDuration(control.dragging ? control.preview : control.value)
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
