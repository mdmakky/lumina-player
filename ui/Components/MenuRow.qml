import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// One selectable row of a PopupPanel: [indicator/icon slot] primary + optional secondary
// line [hint]. Emits triggered() and closes its panel unless closeOnClick is false.
AbstractButton {
    id: row

    property string secondary: ""
    property string hint: ""          // right-aligned shortcut hint
    property string iconSource: ""       // svg shown in the slot instead of the indicator
    property string mark: "none" // "radio" | "check" | "none"
    property bool selected: false
    property bool closeOnClick: true
    signal triggered()

    Layout.fillWidth: true
    implicitHeight: secondary !== "" ? 46 : 36
    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    Accessible.role: mark === "radio" ? Accessible.RadioButton : Accessible.MenuItem
    Accessible.name: text + (secondary !== "" ? ", " + secondary : "")
    Accessible.checked: selected

    function closePanel() {
        for (let p = row.parent; p; p = p.parent) {
            if (p.panelRef !== undefined) { p.panelRef.close(); return }
        }
    }
    onClicked: { triggered(); if (closeOnClick) closePanel() }
    Keys.onReturnPressed: click()
    Keys.onEnterPressed: click()

    contentItem: RowLayout {
        spacing: 10
        opacity: row.enabled ? 1 : 0.4

        Item {
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
            Layout.alignment: Qt.AlignVCenter
            Image {
                visible: row.iconSource !== ""
                anchors.centerIn: parent
                source: Theme.isLight ? row.iconSource.replace(".svg", "-dark.svg") : row.iconSource
                sourceSize: Qt.size(16, 16)
                opacity: 0.85
            }
            Rectangle { // radio
                visible: row.iconSource === "" && row.mark === "radio"
                anchors.centerIn: parent
                width: 16; height: 16; radius: 8
                color: "transparent"
                border.width: 1.5
                border.color: row.selected ? Theme.accent : Theme.disabledText
                Rectangle {
                    anchors.centerIn: parent
                    width: 8; height: 8; radius: 4
                    color: Theme.accent
                    visible: row.selected
                }
            }
            Text { // check
                visible: row.iconSource === "" && row.mark === "check" && row.selected
                anchors.centerIn: parent
                text: "✓"
                color: Theme.accent
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 1
            Text {
                Layout.fillWidth: true
                text: row.text
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: row.secondary !== ""
                text: row.secondary
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                elide: Text.ElideRight
            }
        }

        Text {
            visible: row.hint !== ""
            text: row.hint
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            Layout.alignment: Qt.AlignVCenter
        }
    }

    background: Rectangle {
        radius: Theme.radius - 2
        color: row.down ? Theme.surfacePressed : (row.hovered || row.visualFocus ? Theme.surfaceHover : "transparent")
        border.width: row.visualFocus ? 1.5 : 0
        border.color: Theme.focusRing
        Behavior on color { ColorAnimation { duration: Theme.animationFast } }
    }
}
