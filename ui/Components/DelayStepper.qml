import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// "Label   [-]  +100 ms  [+]" row for sync delays. Never closes its panel.
// `seconds` is the current delay; stepped(delta) is emitted per click (auto-repeats on hold).
Item {
    id: stepper

    property string label: ""
    property real seconds: 0
    property real step: 0.1
    signal stepped(real delta)

    Layout.fillWidth: true
    implicitHeight: 40

    readonly property int ms: Math.round(seconds * 1000)
    readonly property string valueText: (ms > 0 ? "+" : (ms < 0 ? "−" : "")) + Math.abs(ms) + " ms"

    component StepButton: AbstractButton {
        id: b
        property string glyph
        implicitWidth: 30
        implicitHeight: 30
        autoRepeat: true
        autoRepeatDelay: 400
        autoRepeatInterval: 90
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.role: Accessible.Button
        Accessible.name: glyph === "+" ? qsTr("Increase") : qsTr("Decrease")
        contentItem: Text {
            text: b.glyph
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: 18
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            radius: Theme.radius - 2
            color: b.down ? Theme.surfacePressed : (b.hovered ? Theme.surfaceHover : Theme.elevated)
            border.width: 1
            border.color: b.visualFocus ? Theme.focusRing : Theme.border
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 4
        spacing: 6
        Text {
            Layout.fillWidth: true
            text: stepper.label
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            elide: Text.ElideRight
        }
        StepButton {
            objectName: stepper.objectName + "Minus"
            glyph: "−"
            onClicked: stepper.stepped(-stepper.step)
        }
        Text {
            objectName: stepper.objectName + "Value"
            Layout.preferredWidth: 64
            text: stepper.valueText
            color: stepper.ms === 0 ? Theme.secondaryText : Theme.accentHover
            font.family: Theme.fontMono
            font.pixelSize: Theme.fontSizeMedium
            horizontalAlignment: Text.AlignHCenter
        }
        StepButton {
            objectName: stepper.objectName + "Plus"
            glyph: "+"
            onClicked: stepper.stepped(stepper.step)
        }
    }
}
