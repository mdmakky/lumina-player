import QtQuick
import Lumina.Player

// On/off setting. `checked` is driven by the owner; `toggled(value)` asks it to change.
SettingRow {
    id: row

    property string name
    property bool checked: false
    signal toggled(bool value)

    Item {
        id: sw
        objectName: row.name
        implicitWidth: 44
        implicitHeight: 24
        activeFocusOnTab: true
        Accessible.role: Accessible.CheckBox
        Accessible.name: row.title
        Accessible.checked: row.checked
        Accessible.onToggleAction: row.toggled(!row.checked)
        Accessible.onPressAction: row.toggled(!row.checked)

        Keys.onSpacePressed: row.toggled(!row.checked)
        Keys.onReturnPressed: row.toggled(!row.checked)

        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: row.checked ? Theme.accent : Theme.trackIdle
            border.width: sw.activeFocus ? 2 : 0
            border.color: Theme.focusRing
            Behavior on color { ColorAnimation { duration: Theme.animationFast } }
            Rectangle {
                x: row.checked ? parent.width - width - 3 : 3
                y: 3
                width: 18
                height: 18
                radius: 9
                color: Theme.accentText
                Behavior on x { NumberAnimation { duration: Theme.animationFast } }
            }
        }
        MouseArea {
            anchors.fill: parent
            anchors.margins: -8
            cursorShape: Qt.PointingHandCursor
            onClicked: { sw.forceActiveFocus(); row.toggled(!row.checked) }
        }
    }
}
