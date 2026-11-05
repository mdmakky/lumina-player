import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Single-choice pill group. model: [{label, value, id?}]; buttons get objectName name_<id|value>.
Rectangle {
    id: seg

    property string name
    property string accessibleName
    property var model: []
    property var value
    signal picked(var v)

    implicitWidth: row.implicitWidth + 4
    implicitHeight: 32
    radius: Theme.radius
    color: Theme.elevated
    border.color: Theme.border

    Accessible.role: Accessible.RadioButton
    Accessible.name: accessibleName

    Row {
        id: row
        x: 2
        y: 2
        spacing: 2
        Repeater {
            model: seg.model
            delegate: Button {
                id: opt
                required property var modelData
                readonly property bool selected: String(seg.value) === String(modelData.value)
                objectName: seg.name + "_" + (modelData.id !== undefined ? modelData.id : modelData.value)
                height: seg.height - 4
                implicitWidth: Math.max(44, optLabel.implicitWidth + 24)
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                onClicked: seg.picked(modelData.value)
                Accessible.role: Accessible.RadioButton
                Accessible.name: modelData.label
                Accessible.checked: selected
                background: Rectangle {
                    radius: Theme.radius - 2
                    color: opt.selected ? Theme.accentSoft : opt.pressed ? Theme.surfacePressed
                         : opt.hovered ? Theme.surfaceHover : "transparent"
                    border.width: opt.visualFocus || opt.selected ? 1 : 0
                    border.color: opt.visualFocus ? Theme.focusRing : Theme.accent
                }
                contentItem: Text {
                    id: optLabel
                    text: opt.modelData.label
                    color: opt.selected ? Theme.accentHover : Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                    font.weight: opt.selected ? Font.Medium : Font.Normal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
