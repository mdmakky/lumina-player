import QtQuick
import QtQuick.Controls.Basic
import Lumina.Player

// Drop-down choice. model: [{label, value}]; `picked(value)` on selection.
SettingRow {
    id: row

    property string name
    property var model: []
    property var value
    signal picked(var v)

    function indexOfValue(v) {
        for (let i = 0; i < model.length; ++i)
            if (String(model[i].value) === String(v)) return i
        return -1
    }

    ComboBox {
        id: cb
        objectName: row.name
        implicitWidth: 170
        implicitHeight: 32
        model: row.model
        textRole: "label"
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.name: row.title
        onActivated: (i) => row.picked(row.model[i].value)
        onModelChanged: currentIndex = row.indexOfValue(row.value)
        Binding { target: cb; property: "currentIndex"; value: row.indexOfValue(row.value) }

        background: Rectangle {
            radius: Theme.radius
            color: cb.pressed ? Theme.surfacePressed : cb.hovered ? Theme.surfaceHover : Theme.elevated
            border.width: cb.visualFocus ? 2 : 1
            border.color: cb.visualFocus ? Theme.focusRing : Theme.border
        }
        contentItem: Text {
            leftPadding: 12
            rightPadding: 28
            text: cb.displayText
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        indicator: Canvas {
            x: cb.width - width - 10
            y: (cb.height - height) / 2
            width: 10
            height: 6
            property color stroke: Theme.secondaryText
            onStrokeChanged: requestPaint()
            onPaint: {
                const c = getContext("2d")
                c.reset()
                c.strokeStyle = stroke
                c.lineWidth = 1.6
                c.beginPath()
                c.moveTo(1, 1); c.lineTo(5, 5); c.lineTo(9, 1)
                c.stroke()
            }
        }
        delegate: ItemDelegate {
            id: item
            required property var modelData
            required property int index
            width: cb.width - 8
            height: 32
            highlighted: cb.highlightedIndex === index
            contentItem: Text {
                text: item.modelData.label
                color: cb.currentIndex === item.index ? Theme.accentHover : Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                radius: Theme.radius - 2
                color: item.highlighted ? Theme.surfaceHover : "transparent"
            }
        }
        popup: Popup {
            y: cb.height + 4
            width: cb.width
            padding: 4
            implicitHeight: contentItem.implicitHeight + 8
            contentItem: ListView {
                clip: true
                implicitHeight: Math.min(contentHeight, 240)
                model: cb.popup.visible ? cb.delegateModel : null
                currentIndex: cb.highlightedIndex
            }
            background: Rectangle {
                radius: Theme.radius
                color: Theme.elevated
                border.color: Theme.border
            }
        }
    }
}
