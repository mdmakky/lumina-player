import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Lumina.Player

// Appearance > Accent colour: presets, custom hex with live preview, theme JSON import/export.
// Theme.accentBase does the derivation (accent / hover / pressed / soft); "" is the built-in violet.
SettingSection {
    id: sec
    objectName: "accentSection"

    property var dlg // SettingsDialog: owns the Dark/Light/OLED mode
    property string accent: ""   // saved accent, "" = Violet
    property string status: ""
    readonly property var presets: [
        { id: "violet", label: qsTr("Violet"), hex: "" },
        { id: "blue",   label: qsTr("Blue"),   hex: "#3B82F6" },
        { id: "teal",   label: qsTr("Teal"),   hex: "#14B8A6" },
        { id: "green",  label: qsTr("Green"),  hex: "#22C55E" },
        { id: "orange", label: qsTr("Orange"), hex: "#F97316" },
        { id: "rose",   label: qsTr("Rose"),   hex: "#F43F5E" },
        { id: "mono",   label: qsTr("Mono"),   hex: "#9AA0AD" }
    ]
    readonly property color violet: "#8175F5"

    function validHex(t) { return /^#[0-9a-fA-F]{6}$/.test(t) }
    function presetFor(hex) {
        for (const p of presets) if (p.hex.toLowerCase() === hex.toLowerCase()) return p
        return null
    }
    function setAccent(hex) {
        accent = hex
        Theme.accentBase = hex
        player.setSetting("interface/accent", hex)
    }
    function themeJson() {
        const p = presetFor(accent)
        return JSON.stringify({ name: p ? p.label : qsTr("Custom"), mode: Theme.mode,
                                accent: accent === "" ? String(violet).toUpperCase() : accent }, null, 2)
    }
    // Returns "" on success, otherwise a message. Validates before changing anything.
    function applyThemeJson(text) {
        let t
        try { t = JSON.parse(text) } catch (e) { return qsTr("Not a valid theme file.") }
        if (!t || typeof t !== "object" || !validHex(String(t.accent)) || ["dark", "light", "oled"].indexOf(t.mode) < 0)
            return qsTr("Not a valid theme file.")
        setAccent(String(t.accent).toLowerCase() === String(violet).toLowerCase() ? "" : String(t.accent))
        if (dlg) dlg.setThemeMode(t.mode)
        return ""
    }

    title: qsTr("Accent colour")
    Component.onCompleted: {
        accent = player.setting("interface/accent", "")
        if (accent !== "" && !validHex(accent)) accent = ""
        Theme.accentBase = accent
    }

    SettingRow {
        title: qsTr("Accent")
        description: qsTr("Used for highlights, sliders and buttons in every theme.")
        keywords: "colour color accent violet blue teal green orange rose mono highlight"
        stacked: true
        Flow {
            Layout.fillWidth: true
            spacing: Theme.spacingMedium
            Repeater {
                model: sec.presets
                delegate: Item {
                    id: chip
                    required property var modelData
                    readonly property bool current: sec.accent.toLowerCase() === modelData.hex.toLowerCase()
                    objectName: "accentPreset_" + modelData.id
                    width: 74; height: 32
                    activeFocusOnTab: true
                    Accessible.role: Accessible.RadioButton
                    Accessible.name: modelData.label
                    Accessible.checked: current
                    Keys.onSpacePressed: sec.setAccent(modelData.hex)
                    Keys.onReturnPressed: sec.setAccent(modelData.hex)
                    Rectangle {
                        anchors.fill: parent
                        radius: Theme.radius
                        color: chip.current ? Theme.accentSoft : area.containsMouse ? Theme.surfaceHover : "transparent"
                        border.width: chip.activeFocus ? 2 : 1
                        border.color: chip.activeFocus ? Theme.focusRing : chip.current ? Theme.accent : Theme.border
                    }
                    Row {
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 14; height: 14; radius: 7
                            color: chip.modelData.hex === "" ? sec.violet : chip.modelData.hex
                            border.width: 1
                            border.color: Theme.border
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: chip.modelData.label
                            color: Theme.primaryText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }
                    MouseArea {
                        id: area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { sec.setAccent(chip.modelData.hex); hexField.text = chip.modelData.hex }
                    }
                }
            }
        }
    }
    SettingRow {
        title: qsTr("Custom colour")
        description: qsTr("A hex value such as #E11D74. The whole app previews it as you type.")
        keywords: "hex custom rgb color picker"
        RowLayout {
            spacing: Theme.spacingMedium
            Repeater { // preview strip: accent, hover, pressed, soft
                model: [Theme.accent, Theme.accentHover, Theme.accentPressed, Theme.accentSoft]
                delegate: Rectangle {
                    required property var modelData
                    width: 18; height: 18; radius: 4
                    color: modelData
                    border.width: 1
                    border.color: Theme.border
                }
            }
            TextField {
                id: hexField
                objectName: "accentHex"
                implicitWidth: 96
                implicitHeight: 32
                text: sec.accent
                placeholderText: "#8175F5"
                placeholderTextColor: Theme.disabledText
                maximumLength: 7
                color: sec.validHex(text) || text === "" ? Theme.primaryText : Theme.error
                selectByMouse: true
                font.family: Theme.fontMono
                font.pixelSize: Theme.fontSizeMedium
                Accessible.name: qsTr("Custom accent colour")
                background: Rectangle {
                    radius: Theme.radius
                    color: Theme.elevated
                    border.width: hexField.activeFocus ? 2 : 1
                    border.color: hexField.activeFocus ? Theme.focusRing : Theme.border
                }
                onTextEdited: if (sec.validHex(text)) Theme.accentBase = text // live preview, saved on Enter/blur
                onEditingFinished: {
                    if (sec.validHex(text)) sec.setAccent(text)
                    else { Theme.accentBase = sec.accent; text = sec.accent }
                }
            }
        }
    }
    ButtonRow {
        name: "themeExport"
        title: qsTr("Theme file")
        description: sec.status !== "" ? sec.status : qsTr("Save or load the name, mode and accent as a small JSON file.")
        keywords: "import export json share theme"
        buttonText: qsTr("Export…")
        button2Text: qsTr("Import…")
        onClicked: exportDialog.open()
        onClicked2: importDialog.open()
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export theme")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Theme files (*.json)")]
        onAccepted: sec.status = player.writeTextFile(selectedFile, sec.themeJson()) ? qsTr("Theme exported.") : qsTr("Could not write the file.")
    }
    FileDialog {
        id: importDialog
        title: qsTr("Import theme")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Theme files (*.json)")]
        onAccepted: {
            const err = sec.applyThemeJson(player.readTextFile(selectedFile))
            sec.status = err === "" ? qsTr("Theme imported.") : err
            hexField.text = sec.accent
        }
    }
}
