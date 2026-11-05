import QtQuick
import QtQuick.Layouts
import Lumina.Player

// Numeric setting with - / + buttons (objectNames <name>Minus / <name>Plus).
SettingRow {
    id: row

    property string name
    property real value: 0
    property real from: 0
    property real to: 100
    property real step: 1
    property int decimals: 0
    property string suffix: ""
    signal picked(real v)

    RowLayout {
        spacing: 4
        SettingsButton {
            objectName: row.name + "Minus"
            implicitWidth: 32
            text: "−"
            enabled: row.value > row.from
            Accessible.name: qsTr("Decrease %1").arg(row.title)
            onClicked: row.picked(Math.max(row.from, +(row.value - row.step).toFixed(2)))
        }
        Text {
            Layout.preferredWidth: 62
            horizontalAlignment: Text.AlignHCenter
            text: row.value.toFixed(row.decimals) + row.suffix
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            font.features: { "tnum": 1 }
            Accessible.role: Accessible.StaticText
            Accessible.name: row.title + " " + text
        }
        SettingsButton {
            objectName: row.name + "Plus"
            implicitWidth: 32
            text: "+"
            enabled: row.value < row.to
            Accessible.name: qsTr("Increase %1").arg(row.title)
            onClicked: row.picked(Math.min(row.to, +(row.value + row.step).toFixed(2)))
        }
    }
}
