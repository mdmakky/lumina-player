import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Range setting with a value readout. `moved(v)` fires while dragging / on key steps.
SettingRow {
    id: row

    property string name
    property real value: 0
    property real from: 0
    property real to: 100
    property real step: 1
    property string suffix: ""
    signal moved(real v)

    RowLayout {
        spacing: Theme.spacingMedium
        Slider {
            id: slider
            objectName: row.name
            Layout.preferredWidth: 180
            implicitHeight: 28
            from: row.from
            to: row.to
            stepSize: row.step
            snapMode: Slider.SnapAlways
            focusPolicy: Qt.StrongFocus
            Accessible.name: row.title
            onMoved: row.moved(value)
            Binding { target: slider; property: "value"; value: row.value } // survives user drags
            background: Rectangle {
                x: slider.leftPadding
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: slider.availableWidth
                height: 4
                radius: 2
                color: Theme.trackIdle
                Rectangle {
                    width: slider.visualPosition * parent.width
                    height: parent.height
                    radius: 2
                    color: Theme.accent
                }
            }
            handle: Rectangle {
                x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                y: slider.topPadding + slider.availableHeight / 2 - height / 2
                width: 16
                height: 16
                radius: 8
                color: Theme.accentText
                border.width: slider.visualFocus ? 2 : 1
                border.color: slider.visualFocus ? Theme.focusRing : Theme.accent
            }
        }
        Text {
            Layout.preferredWidth: 48
            horizontalAlignment: Text.AlignRight
            text: Math.round(row.value) + row.suffix
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            font.features: { "tnum": 1 }
        }
    }
}
