import QtQuick
import QtQuick.Layouts
import Lumina.Player

// Button row at the foot of a ModalCard: hairline above, buttons right-aligned.
// Not a ModalCard child's layout item itself; place it inside the card body.
ColumnLayout {
    default property alias buttons: row.data
    Layout.fillWidth: true
    Layout.topMargin: Theme.spacingSmall
    spacing: 16
    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.border; opacity: 0.7 }
    RowLayout {
        id: row
        Layout.fillWidth: true
        spacing: Theme.spacingSmall
        Item { Layout.fillWidth: true }
    }
}
