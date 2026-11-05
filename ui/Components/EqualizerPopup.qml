import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// 10-band graphic equalizer (+/-12 dB) with presets; state lives in the engine.
Popup {
    id: popup

    readonly property var bands: ["31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"]
    readonly property var presets: [
        { name: "Flat",       gains: [0, 0, 0, 0, 0, 0, 0, 0, 0, 0] },
        { name: "Bass boost", gains: [6, 5, 4, 2, 0, 0, 0, 0, 0, 0] },
        { name: "Vocal",      gains: [-2, -2, -1, 1, 3, 4, 3, 1, 0, -1] },
        { name: "Treble",     gains: [0, 0, 0, 0, 0, 1, 3, 5, 6, 6] },
        { name: "Loudness",   gains: [5, 4, 1, 0, -1, -1, 0, 2, 4, 5] }
    ]
    function matches(g) {
        const cur = player.engine.eqGains
        for (let i = 0; i < 10; ++i)
            if (Math.abs(cur[i] - g[i]) > 0.05) return false
        return true
    }

    focus: true
    padding: Theme.spacingLarge
    background: Rectangle {
        color: Theme.panelOverlay
        border.color: Theme.border
        radius: Theme.radius
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingMedium

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: qsTr("Equalizer")
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                Layout.fillWidth: true
            }
            Switch {
                objectName: "eqSwitch"
                checked: player.engine.eqEnabled
                onToggled: player.engine.eqEnabled = checked
                id: eqSw
                Accessible.name: qsTr("Equalizer enabled")
                indicator: Rectangle {
                    implicitWidth: 40; implicitHeight: 22
                    x: eqSw.leftPadding; y: parent.height / 2 - height / 2
                    radius: height / 2
                    color: eqSw.checked ? Theme.accent : Theme.trackIdle
                    Rectangle {
                        x: eqSw.checked ? parent.width - width - 3 : 3
                        y: 3; width: 16; height: 16; radius: 8
                        color: Theme.accentText
                        Behavior on x { NumberAnimation { duration: Theme.animationFast } }
                    }
                }
            }
        }

        RowLayout {
            spacing: Theme.spacingSmall
            Repeater {
                model: popup.presets
                delegate: Button {
                    required property var modelData
                    objectName: "eqPreset_" + modelData.name
                    readonly property bool on: popup.matches(modelData.gains)
                    text: modelData.name
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    onClicked: { player.engine.setEqualizer(modelData.gains); player.engine.eqEnabled = true }
                    background: Rectangle {
                        radius: height / 2
                        color: parent.on ? Theme.accent : parent.hovered ? Theme.surfaceHover : "transparent"
                        border.color: parent.on ? Theme.accent : Theme.border
                    }
                    contentItem: Text {
                        text: parent.text
                        font: parent.font
                        color: parent.on ? Theme.background : Theme.primaryText
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }
        }

        RowLayout {
            spacing: Theme.spacingSmall
            opacity: player.engine.eqEnabled ? 1 : 0.5
            Repeater {
                model: popup.bands
                delegate: ColumnLayout {
                    required property string modelData
                    required property int index
                    spacing: 2
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: Math.round(s.value)
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                    Slider {
                        id: s
                        objectName: "eqBand_" + index
                        orientation: Qt.Vertical
                        Layout.alignment: Qt.AlignHCenter
                        Layout.preferredHeight: 150
                        Layout.preferredWidth: 34
                        from: -12; to: 12; stepSize: 1
                        value: player.engine.eqGains[index]
                        onMoved: {
                            const g = player.engine.eqGains.slice()
                            g[index] = value
                            player.engine.setEqualizer(g)
                        }
                        background: Rectangle {
                            x: s.leftPadding + s.availableWidth / 2 - width / 2
                            y: s.topPadding
                            width: 3
                            height: s.availableHeight
                            radius: 2
                            color: Theme.trackIdle
                            Rectangle {
                                y: Math.min(parent.height / 2, s.visualPosition * parent.height)
                                width: parent.width
                                height: Math.abs(s.visualPosition - 0.5) * parent.height
                                color: Theme.accent
                            }
                        }
                        handle: Rectangle {
                            x: s.leftPadding + s.availableWidth / 2 - width / 2
                            y: s.topPadding + s.visualPosition * (s.availableHeight - height)
                            width: 12; height: 12; radius: 6
                            color: Theme.primaryText
                        }
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: modelData
                        color: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }
        }

        Button {
            objectName: "eqReset"
            Layout.alignment: Qt.AlignRight
            text: qsTr("Reset")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            onClicked: player.engine.setEqualizer([0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
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
