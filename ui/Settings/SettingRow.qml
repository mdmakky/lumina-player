import QtQuick
import QtQuick.Layouts
import Lumina.Player

// One setting: title + short description on the left, the control (the row's
// children) on the right, or below when `stacked`. Participates in global search.
FocusScope {
    id: row

    property string title
    property string description
    property string keywords            // extra search terms
    property bool restartNeeded: false
    property bool stacked: false
    default property alias control: slot.data

    readonly property bool matched: SettingsState.q === ""
        || (title + " " + description + " " + keywords).toLowerCase().indexOf(SettingsState.q) >= 0

    visible: matched
    Layout.fillWidth: true
    implicitHeight: layout.implicitHeight + 28

    Accessible.role: Accessible.Grouping
    Accessible.name: title
    Accessible.description: description

    onActiveFocusChanged: if (activeFocus) SettingsState.reveal(row)

    Rectangle { // hover wash
        anchors.fill: parent
        color: hover.hovered ? Theme.surfaceHover : "transparent"
        opacity: 0.6
    }
    Rectangle { // divider above every row but the first
        visible: row.y > 1
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: Theme.border
        opacity: 0.7
    }
    Rectangle { // keyboard focus marker
        visible: row.activeFocus
        width: 3
        height: parent.height - 16
        anchors.verticalCenter: parent.verticalCenter
        radius: 1.5
        color: Theme.focusRing
    }
    HoverHandler { id: hover }

    GridLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.spacingLarge
        anchors.verticalCenter: parent.verticalCenter
        columns: row.stacked ? 1 : 2
        columnSpacing: Theme.spacingLarge
        rowSpacing: Theme.spacingMedium

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 3
            RowLayout {
                spacing: Theme.spacingMedium
                Text {
                    text: row.title
                    color: Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                    font.weight: Font.Medium
                }
                Rectangle {
                    visible: row.restartNeeded
                    implicitWidth: badgeText.implicitWidth + 12
                    implicitHeight: badgeText.implicitHeight + 4
                    radius: height / 2
                    color: Theme.accentSoft
                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: qsTr("Restart needed")
                        color: Theme.accentHover
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }
            Text {
                visible: row.description !== ""
                Layout.fillWidth: true
                text: row.description
                wrapMode: Text.WordWrap
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        RowLayout {
            id: slot
            Layout.alignment: row.stacked ? (Qt.AlignLeft | Qt.AlignVCenter) : (Qt.AlignRight | Qt.AlignVCenter)
            Layout.fillWidth: row.stacked
            spacing: Theme.spacingSmall
        }
    }
}
