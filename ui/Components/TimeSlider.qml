import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Seek timeline. While dragging, issues coalesced keyframe seeks for a live
// preview, then one exact seek on release.
Slider {
    id: control

    property double preview: 0
    readonly property bool dragging: pressed
    readonly property bool active: hovered || pressed

    from: 0
    to: player.engine.duration > 0 ? player.engine.duration : 1
    value: dragging ? preview : player.engine.position
    onMoved: { preview = value; player.engine.seekPreview(value) }

    onPressedChanged: if (!pressed) player.engine.seek(preview)

    hoverEnabled: true
    implicitWidth: 200
    implicitHeight: 32
    Accessible.name: qsTr("Seek")
    Accessible.description: qsTr("Playback position")

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.availableWidth
        height: control.active ? 6 : 3
        radius: height / 2
        color: Theme.videoTrackIdle
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
                color: Theme.videoText
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
                color: Theme.videoTextSecondary
            }
        }
    }

    handle: Rectangle {
        x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        width: control.active ? 14 : 0
        height: width
        radius: width / 2
        color: Theme.videoText
        border.width: 2
        border.color: Theme.accent
        Behavior on width { NumberAnimation { duration: Theme.animationFast; easing.type: Easing.OutCubic } }
    }

    // Hover/drag preview: time under the pointer (or the drag target) and, when
    // available, a frame from the background thumbnailer. Works without a thumb.
    HoverHandler { id: hover }
    readonly property double hoverTime: availableWidth > 0
        ? Math.max(0, Math.min(1, (hover.point.position.x - leftPadding) / availableWidth)) * to : 0
    readonly property double tipTime: dragging ? preview : hoverTime
    readonly property real tipX: dragging ? leftPadding + visualPosition * availableWidth : hover.point.position.x
    property string thumbUrl: ""
    property int wantedBucket: -1
    readonly property string chapterTitle: {
        let t = ""
        if (player.engine.duration > 0)
            for (const c of player.engine.chapters)
                if (c.time <= tipTime) t = c.title
        return t
    }
    function wantThumb() {
        if (!active || !player.thumbs.available) { thumbUrl = ""; return }
        wantedBucket = player.thumbs.bucketFor(tipTime)
        const u = player.thumbs.request(tipTime)
        if (u !== "") thumbUrl = u
    }
    onTipTimeChanged: wantThumb()
    onActiveChanged: { thumbUrl = ""; wantThumb() }
    Connections {
        target: player.thumbs
        function onReady(bucket) { if (bucket === control.wantedBucket) control.thumbUrl = player.thumbs.urlFor(bucket) }
        function onAvailableChanged() { control.thumbUrl = ""; control.wantThumb() }
    }

    ThemedToolTip {
        objectName: "timelineTip"
        delay: 0
        parent: control
        visible: control.hovered || control.dragging
        // keep the whole tip inside the window
        x: Math.max(4 - control.mapToItem(null, 0, 0).x,
                    Math.min(control.tipX - width / 2,
                             control.Window.width - control.mapToItem(null, 0, 0).x - width - 4))
        y: -height - 10
        padding: 6
        // Neutral near-black card (matches the video chrome) instead of the grey-blue panel color.
        background: Rectangle {
            radius: 10
            color: Theme.videoCard
            border.width: 1
            border.color: Theme.videoCardBorder
        }
        contentItem: Column {
            spacing: 6
            Item {
                visible: control.thumbUrl !== ""
                anchors.horizontalCenter: parent.horizontalCenter
                width: 200
                height: thumb.implicitWidth > 0 ? Math.round(width * thumb.implicitHeight / thumb.implicitWidth) : 112
                Image {
                    id: thumb
                    objectName: "timelineThumb"
                    anchors.fill: parent
                    source: control.thumbUrl
                    cache: true
                    asynchronous: true
                    smooth: true
                    opacity: status === Image.Ready ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }
                }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: timefmt.formatDuration(control.tipTime)
                color: Theme.videoText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                font.weight: Font.Medium
            }
            Text {
                visible: control.chapterTitle !== ""
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(implicitWidth, 200)
                elide: Text.ElideRight
                text: control.chapterTitle
                color: Theme.videoTextSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }
        }
    }
}
