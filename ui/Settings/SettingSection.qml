import QtQuick
import QtQuick.Layouts
import Lumina.Player

// Titled card holding a group of rows. Hidden while a search matches none of them.
ColumnLayout {
    id: sec

    property string title
    default property alias rows: col.data

    readonly property bool matched: {
        col.children.length
        for (let i = 0; i < col.children.length; ++i)
            if (col.children[i].matched === true) return true
        return false
    }

    visible: matched
    Layout.fillWidth: true
    spacing: Theme.spacingSmall

    Text {
        visible: sec.title !== ""
        Layout.leftMargin: 4
        text: sec.title.toUpperCase()
        color: Theme.secondaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeSmall
        font.weight: Font.DemiBold
        font.letterSpacing: 0.6
        Accessible.role: Accessible.Heading
        Accessible.name: sec.title
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: col.implicitHeight
        radius: Theme.radiusLarge
        color: SettingsState.card
        border.color: Theme.border
        clip: true
        ColumnLayout {
            id: col
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 0
        }
    }
}
