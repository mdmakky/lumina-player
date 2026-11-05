import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Modal settings overlay: category nav on the left, content on the right.
// Values persist through player.setSetting and apply live via the engine.
Item {
    id: dlg

    property bool opened: false
    property int category: 0

    // Current values (loaded from QSettings at startup so Main.qml can use hideDelay).
    property string hwdec: "auto-safe"
    property bool resume: true
    property int defaultVolume: 100
    property int subSize: 55
    property string subColor: "#FFFFFF"
    property real subBorder: 3
    property int subPos: 100
    property real hideDelay: 2.6

    readonly property var colorPresets: [
        { name: qsTr("White"),  value: "#FFFFFF" },
        { name: qsTr("Yellow"), value: "#FFE45C" },
        { name: qsTr("Cyan"),   value: "#6FE3F5" },
        { name: qsTr("Green"),  value: "#7DE38D" }
    ]

    function set(key, value) { player.setSetting(key, value) }

    function setHwdec(v) { hwdec = v; set("playback/hwdec", v); player.engine.setHwdec(v) }
    function setResume(v) { resume = v; set("playback/resume", v) }
    function setDefaultVolume(v) { defaultVolume = v; set("playback/defaultVolume", v); player.engine.setVolume(v) }
    function setSubSize(v) { subSize = v; set("subtitles/fontSize", v); player.engine.setSubtitleFontSize(v) }
    function setSubColor(v) { subColor = v; set("subtitles/color", v); player.engine.setSubtitleColor(v) }
    function setSubBorder(v) { subBorder = v; set("subtitles/borderSize", v); player.engine.setSubtitleBorderSize(v) }
    function setSubPos(v) { subPos = v; set("subtitles/position", v); player.engine.setSubtitlePosition(v) }
    function setHideDelay(v) { hideDelay = v; set("interface/hideDelay", v) }

    function open() { opened = true }
    function close() { opened = false }

    Component.onCompleted: {
        hwdec = player.setting("playback/hwdec", "auto-safe")
        const r = player.setting("playback/resume", true)
        resume = r === true || r === "true"
        defaultVolume = Number(player.setting("playback/defaultVolume", player.engine.volume))
        subSize = Number(player.setting("subtitles/fontSize", 55))
        subColor = player.setting("subtitles/color", "#FFFFFF")
        subBorder = Number(player.setting("subtitles/borderSize", 3))
        subPos = Number(player.setting("subtitles/position", 100))
        hideDelay = Number(player.setting("interface/hideDelay", 2.6))
    }

    anchors.fill: parent
    visible: opacity > 0
    opacity: opened ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }

    Shortcut { sequence: "Esc"; enabled: dlg.opened; onActivated: dlg.close() }

    // Scrim: swallows clicks; clicking outside the panel closes.
    Rectangle {
        anchors.fill: parent
        color: Theme.scrimSoft
        MouseArea { anchors.fill: parent; onClicked: dlg.close() }
    }

    // --- small themed building blocks ----------------------------------------
    component Section: Text {
        Layout.fillWidth: true
        color: Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeHeading
        font.weight: Font.DemiBold
    }

    // Label (+ optional hint) on the left; the control is the row's last child.
    component Field: RowLayout {
        id: field
        property string label
        property string hint
        Layout.fillWidth: true
        spacing: Theme.spacingLarge
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Text {
                text: field.label
                color: Theme.primaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
            }
            Text {
                visible: field.hint !== ""
                text: field.hint
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    // Pill button used by segmented choices and steppers.
    component Pill: Button {
        id: pill
        property bool active: false
        implicitHeight: 30
        implicitWidth: Math.max(34, pillLabel.implicitWidth + 24)
        hoverEnabled: true
        background: Rectangle {
            radius: Theme.radius
            color: pill.active ? Theme.accentSoft
                 : pill.pressed ? Theme.surfacePressed
                 : pill.hovered ? Theme.surfaceHover : Theme.elevated
            border.width: pill.visualFocus || pill.active ? 1 : 0
            border.color: pill.visualFocus ? Theme.focusRing : Theme.accent
        }
        contentItem: Text {
            id: pillLabel
            text: pill.text
            color: pill.active ? Theme.accentHover : Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    component Stepper: RowLayout {
        id: st
        property real value
        property real from
        property real to
        property real step: 1
        property int decimals: 0
        property string suffix: ""
        signal picked(real v)
        spacing: 4
        Pill {
            objectName: st.objectName + "Minus"
            text: "−"
            enabled: st.value > st.from
            onClicked: st.picked(Math.max(st.from, +(st.value - st.step).toFixed(2)))
        }
        Text {
            Layout.preferredWidth: 56
            horizontalAlignment: Text.AlignHCenter
            text: st.value.toFixed(st.decimals) + st.suffix
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            font.features: { "tnum": 1 }
        }
        Pill {
            objectName: st.objectName + "Plus"
            text: "+"
            enabled: st.value < st.to
            onClicked: st.picked(Math.min(st.to, +(st.value + st.step).toFixed(2)))
        }
    }

    // --- panel -----------------------------------------------------------------
    Rectangle {
        id: panel
        objectName: "settingsPanel"
        anchors.centerIn: parent
        width: Math.min(700, parent.width - 48)
        height: Math.min(460, parent.height - 48)
        radius: Theme.radiusLarge
        color: Theme.panelOverlay
        border.color: Theme.border
        scale: dlg.opened ? 1 : 0.97
        Behavior on scale { NumberAnimation { duration: Theme.animationMedium; easing.type: Easing.OutCubic } }

        MouseArea { anchors.fill: parent } // keep clicks inside the panel

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // Left nav
            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 170
                color: Theme.surface
                radius: Theme.radiusLarge
                Rectangle { // square off the right edge
                    anchors.right: parent.right; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.radius; color: parent.color
                }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMedium
                    spacing: 2
                    Text {
                        Layout.margins: Theme.spacingMedium
                        text: qsTr("Settings")
                        color: Theme.primaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeLarge
                        font.weight: Font.DemiBold
                    }
                    Repeater {
                        model: [qsTr("Playback"), qsTr("Subtitles"), qsTr("Interface")]
                        delegate: Button {
                            id: nav
                            required property int index
                            required property string modelData
                            objectName: "settingsNav_" + index
                            Layout.fillWidth: true
                            implicitHeight: 36
                            hoverEnabled: true
                            text: modelData
                            onClicked: dlg.category = index
                            background: Rectangle {
                                radius: Theme.radius
                                color: dlg.category === nav.index ? Theme.accentSoft
                                     : nav.hovered ? Theme.surfaceHover : "transparent"
                                border.width: nav.visualFocus ? 2 : 0
                                border.color: Theme.focusRing
                            }
                            contentItem: Text {
                                text: nav.text
                                leftPadding: Theme.spacingMedium
                                color: dlg.category === nav.index ? Theme.accentHover : Theme.primaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeMedium
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // Content
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Theme.spacingLarge * 1.5
                currentIndex: dlg.category

                // Playback
                ColumnLayout {
                    spacing: Theme.spacingLarge
                    Section { text: qsTr("Playback") }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Hardware decoding")
                        hint: qsTr("Auto-safe uses only well-tested decoders. Off is the most compatible.")
                        RowLayout {
                            spacing: 4
                            Repeater {
                                model: [
                                    { label: qsTr("Auto-safe"), value: "auto-safe" },
                                    { label: qsTr("Off"), value: "no" },
                                    { label: qsTr("Auto"), value: "auto" }
                                ]
                                delegate: Pill {
                                    required property var modelData
                                    objectName: "hwdecOption_" + modelData.value
                                    text: modelData.label
                                    active: dlg.hwdec === modelData.value
                                    onClicked: dlg.setHwdec(modelData.value)
                                }
                            }
                        }
                    }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Remember playback position")
                        hint: qsTr("Resume files where you left off.")
                        Switch {
                            id: resumeSwitch
                            objectName: "resumeSwitch"
                            checked: dlg.resume
                            onToggled: dlg.setResume(checked)
                            indicator: Rectangle {
                                implicitWidth: 40; implicitHeight: 22
                                x: resumeSwitch.leftPadding; y: parent.height / 2 - height / 2
                                radius: height / 2
                                color: resumeSwitch.checked ? Theme.accent : Theme.trackIdle
                                Rectangle {
                                    x: resumeSwitch.checked ? parent.width - width - 3 : 3
                                    y: 3; width: 16; height: 16; radius: 8
                                    color: Theme.primaryText
                                    Behavior on x { NumberAnimation { duration: Theme.animationFast } }
                                }
                            }
                        }
                    }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Default volume")
                        hint: qsTr("Volume used when Lumina starts.")
                        Stepper {
                            objectName: "defaultVolume"
                            value: dlg.defaultVolume; from: 0; to: 150; step: 5; suffix: "%"
                            onPicked: (v) => dlg.setDefaultVolume(v)
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Subtitles
                ColumnLayout {
                    spacing: Theme.spacingLarge
                    Section { text: qsTr("Subtitles") }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Font size")
                        Stepper {
                            objectName: "subSize"
                            value: dlg.subSize; from: 20; to: 120; step: 5
                            onPicked: (v) => dlg.setSubSize(v)
                        }
                    }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Color")
                        RowLayout {
                            spacing: 4
                            Repeater {
                                model: dlg.colorPresets
                                delegate: Pill {
                                    required property var modelData
                                    objectName: "subColor_" + modelData.value.substring(1)
                                    text: modelData.name
                                    active: dlg.subColor.toUpperCase() === modelData.value
                                    onClicked: dlg.setSubColor(modelData.value)
                                }
                            }
                        }
                    }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Border size")
                        Stepper {
                            objectName: "subBorder"
                            value: dlg.subBorder; from: 0; to: 8; step: 0.5; decimals: 1
                            onPicked: (v) => dlg.setSubBorder(v)
                        }
                    }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Vertical position")
                        hint: qsTr("0 is the top of the video, 100 the bottom.")
                        Stepper {
                            objectName: "subPos"
                            value: dlg.subPos; from: 0; to: 100; step: 5
                            onPicked: (v) => dlg.setSubPos(v)
                        }
                    }
                    Item { Layout.fillHeight: true }
                }

                // Interface
                ColumnLayout {
                    spacing: Theme.spacingLarge
                    Section { text: qsTr("Interface") }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Hide controls after")
                        hint: qsTr("Idle time before the controls fade out during playback.")
                        Stepper {
                            objectName: "hideDelay"
                            value: dlg.hideDelay; from: 1; to: 10; step: 0.5; decimals: 1; suffix: " s"
                            onPicked: (v) => dlg.setHideDelay(v)
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }

        Button {
            id: closeBtn
            objectName: "settingsCloseButton"
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.spacingMedium
            width: 30; height: 30
            hoverEnabled: true
            onClicked: dlg.close()
            background: Rectangle {
                radius: Theme.radius
                color: closeBtn.hovered ? Theme.surfaceHover : "transparent"
            }
            contentItem: Text {
                text: "✕"
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }
    }
}
