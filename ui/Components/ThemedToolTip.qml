import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Tooltip in the shared panel style (hairline border, soft shadow, quick fade).
// Set `text` for the common case, or supply your own contentItem.
ToolTip {
    id: tip

    delay: 500
    padding: 7
    leftPadding: 10
    rightPadding: 10

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.animationFast } }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.animationFast } }

    contentItem: Text {
        text: tip.text
        color: Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeSmall
        wrapMode: Text.Wrap
    }
    background: Surface { radius: Theme.radius - 2; color: Theme.elevated }
}
