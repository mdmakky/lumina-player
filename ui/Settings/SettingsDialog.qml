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
    property string themeMode: "dark"
    property bool reducedMotion: false
    // Read-only probes for the UI self-test.
    readonly property string activeTheme: Theme.mode
    readonly property int animFast: Theme.animationFast
    readonly property int animMedium: Theme.animationMedium

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
    function setThemeMode(v) { themeMode = v; Theme.mode = v; set("interface/theme", v) }
    function setReducedMotion(v) { reducedMotion = v; Theme.reducedMotion = v; set("interface/reducedMotion", v) }

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
        themeMode = player.setting("interface/theme", "dark")
        Theme.mode = themeMode
        const rm = player.setting("interface/reducedMotion", false)
        reducedMotion = rm === true || rm === "true"
        Theme.reducedMotion = reducedMotion
    }

    anchors.fill: parent
    visible: opacity > 0
    opacity: opened ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.animationMedium } }

    Shortcut { sequence: "Esc"; enabled: dlg.opened && !shortcuts.capturing; onActivated: dlg.close() }

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
                        model: [qsTr("Playback"), qsTr("Subtitles"), qsTr("Interface"), qsTr("Shortcuts")]
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
                                    color: Theme.accentText
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
                        label: qsTr("Theme")
                        hint: qsTr("Video controls stay dark in every theme.")
                        RowLayout {
                            spacing: 4
                            Repeater {
                                model: [
                                    { label: qsTr("Dark"), value: "dark" },
                                    { label: qsTr("Light"), value: "light" },
                                    { label: qsTr("OLED black"), value: "oled" }
                                ]
                                delegate: Pill {
                                    required property var modelData
                                    objectName: "themeOption_" + modelData.value
                                    text: modelData.label
                                    active: dlg.themeMode === modelData.value
                                    onClicked: dlg.setThemeMode(modelData.value)
                                }
                            }
                        }
                    }
                    Field {
                        Layout.fillWidth: true
                        label: qsTr("Reduce motion")
                        hint: qsTr("Turns off fades and slide animations.")
                        Switch {
                            id: motionSwitch
                            objectName: "reducedMotionSwitch"
                            checked: dlg.reducedMotion
                            onToggled: dlg.setReducedMotion(checked)
                            indicator: Rectangle {
                                implicitWidth: 40; implicitHeight: 22
                                x: motionSwitch.leftPadding; y: parent.height / 2 - height / 2
                                radius: height / 2
                                color: motionSwitch.checked ? Theme.accent : Theme.trackIdle
                                Rectangle {
                                    x: motionSwitch.checked ? parent.width - width - 3 : 3
                                    y: 3; width: 16; height: 16; radius: 8
                                    color: Theme.accentText
                                    Behavior on x { NumberAnimation { duration: Theme.animationFast } }
                                }
                            }
                        }
                    }
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

                // Shortcuts
                ColumnLayout {
                    id: scPage
                    spacing: Theme.spacingMedium

                    property string capId: ""      // action being rebound
                    property string pendingSeq: "" // candidate waiting on a conflict decision
                    property string pendingWith: ""
                    property string query: ""
                    readonly property bool conflicted: pendingWith !== ""

                    // Flat model: category header rows + matching action rows.
                    readonly property var rows: {
                        const q = query.trim().toLowerCase()
                        const out = []
                        let cat = ""
                        for (const a of shortcuts.actions) {
                            if (q && (a.name + " " + a.category + " " + a.key).toLowerCase().indexOf(q) < 0)
                                continue
                            if (a.category !== cat) { cat = a.category; out.push({ header: true, name: cat }) }
                            out.push(a)
                        }
                        return out
                    }

                    function endCapture() { capId = ""; pendingSeq = ""; pendingWith = ""; shortcuts.capturing = false }
                    function beginCapture(id) {
                        capId = id; pendingSeq = ""; pendingWith = ""
                        shortcuts.capturing = true
                        captureKeys.forceActiveFocus()
                    }
                    function commit(swap) {
                        if (swap) shortcuts.swap(capId, pendingSeq)
                        else shortcuts.setSequence(capId, pendingSeq)
                        endCapture()
                    }
                    Connections {
                        target: dlg
                        function onOpenedChanged() { if (!dlg.opened) scPage.endCapture() }
                        function onCategoryChanged() { scPage.endCapture() }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Section { text: qsTr("Shortcuts"); Layout.fillWidth: true }
                        Pill {
                            objectName: "shortcutsResetAll"
                            text: qsTr("Reset all")
                            focusPolicy: Qt.NoFocus
                            onClicked: { scPage.endCapture(); shortcuts.resetAll() }
                        }
                    }

                    TextField {
                        id: scSearch
                        objectName: "shortcutSearch"
                        Layout.fillWidth: true
                        placeholderText: qsTr("Search actions or keys")
                        color: Theme.primaryText
                        placeholderTextColor: Theme.secondaryText
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeMedium
                        background: Rectangle {
                            radius: Theme.radius
                            color: Theme.elevated
                            border.width: scSearch.activeFocus ? 1 : 0
                            border.color: Theme.focusRing
                        }
                        onTextChanged: scPage.query = text
                        onActiveFocusChanged: if (activeFocus) scPage.endCapture()
                    }

                    ListView {
                        id: scList
                        objectName: "shortcutList"
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: scPage.rows
                        boundsBehavior: Flickable.StopAtBounds
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Item {
                            id: row
                            required property var modelData
                            readonly property bool isHeader: modelData.header === true
                            readonly property bool capturing: !isHeader && scPage.capId === modelData.id
                            objectName: isHeader ? "" : "shortcutRow_" + modelData.id
                            width: scList.width - 10
                            height: isHeader ? 30 : 34

                            Text {
                                visible: row.isHeader
                                anchors.left: parent.left
                                anchors.bottom: parent.bottom
                                anchors.bottomMargin: 4
                                text: row.isHeader ? row.modelData.name.toUpperCase() : ""
                                color: Theme.secondaryText
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeSmall
                                font.weight: Font.DemiBold
                            }
                            Rectangle {
                                visible: !row.isHeader
                                anchors.fill: parent
                                radius: Theme.radius
                                color: row.capturing ? Theme.accentSoft : rowMouse.containsMouse ? Theme.surfaceHover : "transparent"
                                MouseArea {
                                    id: rowMouse
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onClicked: scPage.beginCapture(row.modelData.id)
                                }
                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: Theme.spacingMedium
                                    anchors.rightMargin: Theme.spacingSmall
                                    spacing: Theme.spacingMedium
                                    Text {
                                        Layout.fillWidth: true
                                        text: row.isHeader ? "" : row.modelData.name
                                        elide: Text.ElideRight
                                        color: Theme.primaryText
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontSizeMedium
                                    }
                                    Pill {
                                        objectName: row.isHeader ? "" : "shortcutReset_" + row.modelData.id
                                        visible: !row.isHeader && row.modelData.key !== row.modelData.defaultKey
                                        implicitHeight: 24
                                        text: qsTr("Reset")
                                        focusPolicy: Qt.NoFocus
                                        onClicked: { scPage.endCapture(); shortcuts.reset(row.modelData.id) }
                                    }
                                    Rectangle {
                                        Layout.preferredWidth: Math.max(78, keyText.implicitWidth + 20)
                                        Layout.preferredHeight: 24
                                        radius: Theme.radius
                                        color: Theme.elevated
                                        border.width: row.capturing ? 1 : 0
                                        border.color: Theme.accent
                                        Text {
                                            id: keyText
                                            anchors.centerIn: parent
                                            text: row.isHeader ? ""
                                                : row.capturing ? qsTr("Press keys…")
                                                : row.modelData.key === "" ? qsTr("Unbound") : row.modelData.key
                                            color: row.capturing ? Theme.accentHover
                                                 : row.modelData.key === "" ? Theme.secondaryText : Theme.primaryText
                                            font.family: Theme.fontFamily
                                            font.pixelSize: Theme.fontSizeSmall
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Capture / conflict status line.
                    RowLayout {
                        Layout.fillWidth: true
                        visible: scPage.capId !== ""
                        spacing: Theme.spacingMedium
                        Text {
                            objectName: "shortcutHint"
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            color: scPage.conflicted ? Theme.error : Theme.secondaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeSmall
                            text: scPage.conflicted
                                ? qsTr("%1 is already used by “%2”.").arg(scPage.pendingSeq).arg(shortcuts.nameOf(scPage.pendingWith))
                                : qsTr("Press a key combination. Esc cancels, Backspace clears.")
                        }
                        Pill {
                            objectName: "shortcutSwap"
                            visible: scPage.conflicted
                            text: qsTr("Swap")
                            focusPolicy: Qt.NoFocus
                            onClicked: scPage.commit(true)
                        }
                        Pill {
                            objectName: "shortcutConflictCancel"
                            visible: scPage.conflicted
                            text: qsTr("Cancel")
                            focusPolicy: Qt.NoFocus
                            onClicked: scPage.endCapture()
                        }
                    }

                    // Invisible key sink while rebinding.
                    Item {
                        id: captureKeys
                        focus: false
                        Keys.onShortcutOverride: (event) => event.accepted = scPage.capId !== ""
                        Keys.onPressed: (event) => {
                            if (scPage.capId === "") { event.accepted = false; return }
                            event.accepted = true
                            if (event.key === Qt.Key_Escape) { scPage.endCapture(); return }
                            if (scPage.conflicted) return
                            if (event.key === Qt.Key_Backspace && event.modifiers === Qt.NoModifier) {
                                shortcuts.setSequence(scPage.capId, "")
                                scPage.endCapture()
                                return
                            }
                            const seq = shortcuts.fromKeyEvent(event.key, event.modifiers)
                            if (seq === "") return
                            const other = shortcuts.conflict(scPage.capId, seq)
                            if (other !== "") { scPage.pendingSeq = seq; scPage.pendingWith = other }
                            else { shortcuts.setSequence(scPage.capId, seq); scPage.endCapture() }
                        }
                    }
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
