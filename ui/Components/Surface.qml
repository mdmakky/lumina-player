import QtQuick
import Lumina.Player

// Raised surface shared by every popup, menu and dialog: hairline border, a 1px inner highlight on
// the lit edge, and a soft drop shadow that falls downward (elevation 1 = menu, 2 = modal dialog).
Item {
    id: surf

    property real radius: Theme.radiusLarge
    property color color: Theme.surface
    property int elevation: 1

    Repeater {
        model: 4
        Rectangle {
            required property int index
            readonly property real spread: (index + 1) * 4
            anchors.fill: parent
            anchors.margins: -spread
            anchors.topMargin: -spread + 5 * surf.elevation
            anchors.bottomMargin: -spread - 5 * surf.elevation
            radius: surf.radius + spread
            color: Theme.shadow
            opacity: (0.17 - index * 0.04) * (surf.elevation > 1 ? 1.25 : 1)
        }
    }
    Rectangle {
        anchors.fill: parent
        radius: surf.radius
        color: surf.color
        border.color: Theme.border
        Rectangle {
            anchors.fill: parent
            anchors.margins: 1
            radius: surf.radius - 1
            color: "transparent"
            border.color: Theme.panelHighlight
        }
    }
}
