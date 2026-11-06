import QtQuick
import Lumina.Player

// Placeholder for audio-only files that have neither cover art nor a visualization:
// a soft emblem plus title / artist / album from the file's tags.
Rectangle {
    id: root

    required property var engine
    property bool active: false          // set by the owner (media loaded)

    readonly property bool audioOnly: engine.audioTracks.length > 0 && engine.videoTracks.length === 0
    property var meta: ({})
    function refresh() { meta = engine.fileMetadata() }

    visible: active && audioOnly && engine.visualization === "off"
    color: Theme.background
    Connections {
        target: root.engine
        function onTracksChanged() { root.refresh() }
        function onMediaTitleChanged() { root.refresh() }
    }

    Column {
        anchors.centerIn: parent
        width: Math.min(parent.width - 64, 560)
        spacing: Theme.spacingMedium

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 120; height: 120; radius: 60
            color: Theme.elevated
            border.color: Theme.border
            Text { // a plain note glyph keeps this free of image assets
                anchors.centerIn: parent
                text: "♫"
                color: Theme.accent
                font.pixelSize: 56
                font.family: Theme.fontFamily
            }
        }
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: root.meta.title || root.engine.mediaTitle
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
            font.weight: Font.DemiBold
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.Wrap
        }
        Text {
            width: parent.width
            visible: text !== ""
            horizontalAlignment: Text.AlignHCenter
            text: [root.meta.artist, root.meta.album].filter(x => x).join("  ·  ")
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            elide: Text.ElideRight
        }
    }
}
