import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Brightness / contrast / saturation / gamma / hue sliders (-100..100).
PopupPanel {
    id: popup

    padding: Theme.spacingLarge
    panelWidth: 300

    contentItem: ColumnLayout {
        spacing: Theme.spacingSmall

        // One-tap looks; sliders below stay editable. "Default" = untouched picture.
        readonly property var presets: [
            { name: qsTr("Default"), v: { brightness: 0, contrast: 0, saturation: 0, gamma: 0, hue: 0 } },
            { name: qsTr("Cinema"),  v: { brightness: -4, contrast: 12, saturation: 4, gamma: 0, hue: 0 } },
            { name: qsTr("Vivid"),   v: { brightness: 3, contrast: 15, saturation: 30, gamma: 0, hue: 0 } },
            { name: qsTr("Night"),   v: { brightness: -15, contrast: -5, saturation: -10, gamma: 10, hue: 0 } }
        ]
        function applyPreset(v) { for (const k in v) player.engine.setVideoAdjust(k, v[k]) }
        function isActive(v) {
            const cur = player.engine.videoAdjust
            for (const k in v) if ((cur[k] || 0) !== v[k]) return false
            return true
        }

        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingSmall
            Repeater {
                model: popup.contentItem.presets
                delegate: MenuChip {
                    required property var modelData
                    objectName: "adjustPreset_" + modelData.name
                    text: modelData.name
                    closeOnClick: false
                    selected: popup.contentItem.isActive(modelData.v)
                    onTriggered: popup.contentItem.applyPreset(modelData.v)
                }
            }
        }

        Repeater {
            model: ["brightness", "contrast", "saturation", "gamma", "hue"]
            delegate: RowLayout {
                required property string modelData
                Layout.fillWidth: true
                spacing: Theme.spacingMedium
                Text {
                    Layout.preferredWidth: 80
                    text: ({ brightness: qsTr("Brightness"), contrast: qsTr("Contrast"), saturation: qsTr("Saturation"), gamma: qsTr("Gamma"), hue: qsTr("Hue") })[modelData]
                    color: Theme.secondaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                }
                Slider {
                    id: s
                    objectName: "adjust_" + modelData
                    Layout.fillWidth: true
                    from: -100
                    to: 100
                    stepSize: 1
                    value: player.engine.videoAdjust[modelData]
                    onMoved: player.engine.setVideoAdjust(modelData, Math.round(value))
                    background: Rectangle {
                        x: s.leftPadding
                        y: s.topPadding + s.availableHeight / 2 - height / 2
                        width: s.availableWidth
                        height: 3
                        radius: 2
                        color: Theme.trackIdle
                        Rectangle {
                            x: Math.min(parent.width / 2, s.visualPosition * parent.width)
                            width: Math.abs(s.visualPosition - 0.5) * parent.width
                            height: parent.height
                            color: Theme.accent
                        }
                    }
                    handle: Rectangle {
                        x: s.leftPadding + s.visualPosition * (s.availableWidth - width)
                        y: s.topPadding + s.availableHeight / 2 - height / 2
                        width: 12; height: 12; radius: 6
                        color: Theme.primaryText
                    }
                }
                Text {
                    Layout.preferredWidth: 30
                    horizontalAlignment: Text.AlignRight
                    text: Math.round(s.value)
                    color: Theme.primaryText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                }
            }
        }

        Button {
            objectName: "adjustReset"
            Layout.alignment: Qt.AlignRight
            text: qsTr("Reset")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            onClicked: player.engine.resetVideoAdjust()
            background: Rectangle {
                radius: Theme.radius
                color: parent.hovered ? Theme.surfaceHover : "transparent"
                border.color: Theme.border
            }
            contentItem: Text {
                text: parent.text
                font: parent.font
                color: Theme.primaryText
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
