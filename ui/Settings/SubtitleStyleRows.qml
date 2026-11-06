import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Extra subtitle styling for Settings > Subtitles (spec 7.4): live preview, style presets,
// font, weight, shadow, background box, alignment, text encoding and ASS override.
// Size/colour/border/position stay in SettingsDialog; everything here is one JSON state
// ("subtitles/styleState") plus the derived mpv properties ("subtitles/mpvStyle", applied at startup
// by PlayerController).
ColumnLayout {
    id: root
    objectName: "subStyleRows"

    required property var dlg
    required property var thePlayer   // passed in: not visible as a context property in this file
    readonly property var engine: thePlayer.engine

    Layout.fillWidth: true
    spacing: Theme.spacingLarge

    readonly property bool matched: {
        for (let i = 0; i < children.length; ++i)
            if (children[i].matched === true) return true
        return false
    }

    // ---- state ----
    property string font: ""
    property bool bold: false
    property real shadow: 0
    property bool box: false
    property int boxOpacity: 60
    property string alignX: "center"
    property string alignY: "bottom"
    property string codepage: "auto"
    property bool assOverride: false
    property string presetId: ""
    property var userPresets: []

    readonly property var builtinPresets: [
        { name: qsTr("Default"), size: 40, color: "#FFFFFF", border: 3, bold: false, shadow: 0, box: false, boxOpacity: 60 },
        { name: qsTr("Cinema"), size: 44, color: "#FFE45C", border: 3, bold: true, shadow: 1, box: false, boxOpacity: 60 },
        { name: qsTr("Large print"), size: 64, color: "#FFFFFF", border: 4, bold: true, shadow: 1, box: false, boxOpacity: 60 },
        { name: qsTr("High contrast"), size: 52, color: "#FFFFFF", border: 4, bold: true, shadow: 0, box: true, boxOpacity: 90 },
        { name: qsTr("Soft box"), size: 42, color: "#FFFFFF", border: 0, bold: false, shadow: 0, box: true, boxOpacity: 55 }
    ]
    readonly property var presetModel: {
        const m = [{ label: qsTr("Custom"), value: "" }]
        for (let i = 0; i < builtinPresets.length; ++i) m.push({ label: builtinPresets[i].name, value: "b" + i })
        for (let j = 0; j < userPresets.length; ++j) m.push({ label: userPresets[j].name, value: "u" + j })
        return m
    }
    readonly property var encodings: [
        { label: qsTr("Auto-detect"), value: "auto", id: "auto" },
        { label: "UTF-8", value: "utf-8", id: "utf8" },
        { label: "UTF-16", value: "utf-16", id: "utf16" },
        { label: "Windows-1252 (Western)", value: "windows-1252", id: "cp1252" },
        { label: "ISO-8859-1 (Latin-1)", value: "iso-8859-1", id: "latin1" },
        { label: "Windows-1256 (Arabic)", value: "windows-1256", id: "cp1256" },
        { label: "Windows-1251 (Cyrillic)", value: "windows-1251", id: "cp1251" },
        { label: "Windows-1254 (Turkish)", value: "windows-1254", id: "cp1254" },
        { label: "GBK (Chinese)", value: "gbk", id: "gbk" },
        { label: "Shift_JIS (Japanese)", value: "shift_jis", id: "sjis" },
        { label: "EUC-KR (Korean)", value: "euc-kr", id: "euckr" }
    ]

    // First installed font that carries both Bengali and Latin; "" = mpv's default sans-serif.
    function defaultFont() {
        const fams = Qt.fontFamilies()
        const want = ["Kalpurush", "Noto Sans Bengali", "Noto Serif Bengali", "Nirmala UI", "Bangla MN", "Bangla Sangam MN", "Lohit Bengali"]
        for (let i = 0; i < want.length; ++i)
            if (fams.indexOf(want[i]) >= 0) return want[i]
        return ""
    }

    function hex2(n) { const s = Math.max(0, Math.min(255, Math.round(n))).toString(16); return s.length < 2 ? "0" + s : s }

    function mpvStyle() {
        return {
            "sub-font": font !== "" ? font : "sans-serif",
            "sub-bold": bold ? "yes" : "no",
            "sub-shadow-offset": String(shadow),
            "sub-border-style": box ? "opaque-box" : "outline-and-shadow",
            "sub-outline-color": box ? "#" + hex2(boxOpacity * 2.55) + "000000" : "#FF000000",
            "sub-back-color": box ? "#" + hex2(boxOpacity * 2.55) + "000000" : "#99000000", // also the shadow colour
            "sub-align-x": alignX,
            "sub-align-y": alignY,
            "sub-codepage": codepage,
            "sub-ass-override": assOverride ? "force" : "scale",
            "secondary-sub-pos": "6"
        }
    }

    function save() {
        dlg.set("subtitles/styleState", JSON.stringify({ font: font, bold: bold, shadow: shadow, box: box, boxOpacity: boxOpacity,
            alignX: alignX, alignY: alignY, codepage: codepage, assOverride: assOverride }))
        const st = mpvStyle()
        dlg.set("subtitles/mpvStyle", JSON.stringify(st))
        engine.applySubtitleStyle(st)
    }

    // Changes one field by hand: leaves any preset.
    function edit(key, value) {
        root[key] = value
        presetId = ""
        save()
    }

    function applyPreset(p) {
        bold = p.bold; shadow = p.shadow; box = p.box; boxOpacity = p.boxOpacity
        dlg.setSubSize(p.size); dlg.setSubColor(p.color); dlg.setSubBorder(p.border)
        save()
    }
    function pickPreset(id) {
        presetId = id
        if (id === "") return
        applyPreset(id.charAt(0) === "b" ? builtinPresets[+id.substring(1)] : userPresets[+id.substring(1)])
        presetId = id
    }
    function saveUserPreset(name) {
        name = name.trim()
        if (name === "") return
        const p = { name: name, size: dlg.subSize, color: dlg.subColor, border: dlg.subBorder, bold: bold,
                    shadow: shadow, box: box, boxOpacity: boxOpacity }
        const l = userPresets.filter(function (u) { return u.name !== name })
        l.push(p)
        userPresets = l
        dlg.set("subtitles/userPresets", JSON.stringify(l))
        presetId = "u" + (l.length - 1)
    }
    function deleteUserPreset() {
        if (presetId.charAt(0) !== "u") return
        const l = userPresets.slice()
        l.splice(+presetId.substring(1), 1)
        userPresets = l
        dlg.set("subtitles/userPresets", JSON.stringify(l))
        presetId = ""
    }
    function resetStyle() {
        font = defaultFont(); alignX = "center"; alignY = "bottom"; codepage = "auto"; assOverride = false
        applyPreset(builtinPresets[0])
        presetId = "b0"
    }

    Component.onCompleted: {
        try { userPresets = JSON.parse(thePlayer.setting("subtitles/userPresets", "[]")) } catch (e) { userPresets = [] }
        const raw = thePlayer.setting("subtitles/styleState", "")
        if (raw === "") {
            font = defaultFont()
            save()   // persist the Bengali-capable default on first run
            return
        }
        try {
            const s = JSON.parse(raw)
            font = s.font; bold = s.bold; shadow = s.shadow; box = s.box; boxOpacity = s.boxOpacity
            alignX = s.alignX; alignY = s.alignY; codepage = s.codepage; assOverride = s.assOverride
        } catch (e) { }
    }

    // ------------------------------------------------------------------ Presets + preview
    SettingSection {
        title: qsTr("Style")
        SettingRow {
            title: qsTr("Preview")
            keywords: "sample text bangla bengali look"
            stacked: true
            Rectangle {
                id: prev
                objectName: "subPreview"
                Layout.fillWidth: true
                implicitHeight: 118
                radius: Theme.radius
                color: Theme.videoBackdrop
                border.color: Theme.border
                clip: true
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        GradientStop { position: 0; color: Theme.scrimClear }
                        GradientStop { position: 1; color: Theme.videoTrackIdle }
                    }
                }
                Column {
                    id: lines
                    spacing: 2
                    anchors.horizontalCenter: root.alignX === "center" ? parent.horizontalCenter : undefined
                    anchors.left: root.alignX === "left" ? parent.left : undefined
                    anchors.right: root.alignX === "right" ? parent.right : undefined
                    anchors.margins: 14
                    anchors.top: root.alignY === "top" ? parent.top : undefined
                    anchors.verticalCenter: root.alignY === "center" ? parent.verticalCenter : undefined
                    anchors.bottom: root.alignY === "bottom" ? parent.bottom : undefined
                    Repeater {
                        model: ["The quick brown fox", "বাংলা সাবটাইটেল"]
                        delegate: Item {
                            id: ln
                            required property string modelData
                            readonly property real px: Math.max(11, Math.min(34, root.dlg.subSize * 0.5))
                            width: main.implicitWidth + 12
                            height: main.implicitHeight + 4
                            Rectangle {
                                anchors.fill: parent
                                visible: root.box
                                radius: 3
                                color: Theme.videoBackdrop
                                opacity: root.boxOpacity / 100
                            }
                            Text {
                                visible: root.shadow > 0
                                x: 6 + root.shadow; y: 2 + root.shadow
                                text: ln.modelData
                                color: Theme.videoBackdrop
                                opacity: 0.6
                                font.family: root.font !== "" ? root.font : Theme.fontFamily
                                font.pixelSize: ln.px
                                font.bold: root.bold
                            }
                            Text {
                                id: main
                                x: 6; y: 2
                                text: ln.modelData
                                color: root.dlg.subColor
                                style: root.dlg.subBorder > 0 && !root.box ? Text.Outline : Text.Normal
                                styleColor: Theme.videoBackdrop
                                font.family: root.font !== "" ? root.font : Theme.fontFamily
                                font.pixelSize: ln.px
                                font.bold: root.bold
                            }
                        }
                    }
                }
            }
        }
        ComboRow {
            name: "subPreset"
            title: qsTr("Preset")
            description: qsTr("Applies size, colour, border, weight, shadow and box together.")
            keywords: "theme cinema large print high contrast soft box default"
            model: root.presetModel
            value: root.presetId
            onPicked: (v) => root.pickPreset(v)
        }
        SettingRow {
            title: qsTr("Save current style")
            description: qsTr("Stores the style above as your own preset.")
            keywords: "preset new custom delete remove"
            RowLayout {
                spacing: 6
                TextField {
                    id: presetName
                    objectName: "subPresetName"
                    implicitWidth: 130
                    implicitHeight: 32
                    placeholderText: qsTr("Preset name")
                    color: Theme.primaryText
                    placeholderTextColor: Theme.disabledText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeMedium
                    selectByMouse: true
                    background: Rectangle {
                        radius: Theme.radius
                        color: Theme.elevated
                        border.width: presetName.activeFocus ? 2 : 1
                        border.color: presetName.activeFocus ? Theme.focusRing : Theme.border
                    }
                    onAccepted: { root.saveUserPreset(text); text = "" }
                }
                SettingsButton {
                    objectName: "subPresetSave"
                    text: qsTr("Save")
                    enabled: presetName.text.trim() !== ""
                    onClicked: { root.saveUserPreset(presetName.text); presetName.text = "" }
                }
                SettingsButton {
                    objectName: "subPresetDelete"
                    text: qsTr("Delete")
                    danger: true
                    enabled: root.presetId.charAt(0) === "u"
                    onClicked: root.deleteUserPreset()
                }
            }
        }
    }

    // ------------------------------------------------------------------ Font and effects
    SettingSection {
        title: qsTr("Font and effects")
        SettingRow {
            title: qsTr("Font")
            description: qsTr("Pick one that has Bangla letters, or the text falls back to another font.")
            keywords: "typeface family bangla bengali kalpurush noto"
            SettingsButton {
                id: fontBtn
                objectName: "subFont"
                implicitWidth: 190
                text: root.font !== "" ? root.font : qsTr("System default")
                onClicked: fontPopup.opened ? fontPopup.close() : fontPopup.open()
                Popup {
                    id: fontPopup
                    parent: fontBtn
                    y: fontBtn.height + 4
                    x: fontBtn.width - width
                    width: 260
                    padding: 6
                    modal: false
                    focus: true
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    property var fams: []
                    onAboutToShow: { fams = [""].concat(Qt.fontFamilies()); fontSearch.text = ""; fontSearch.forceActiveFocus() }
                    background: Rectangle { radius: Theme.radius; color: Theme.elevated; border.color: Theme.border }
                    contentItem: ColumnLayout {
                        spacing: 6
                        TextField {
                            id: fontSearch
                            objectName: "subFontSearch"
                            Layout.fillWidth: true
                            implicitHeight: 30
                            placeholderText: qsTr("Search fonts…")
                            color: Theme.primaryText
                            placeholderTextColor: Theme.disabledText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeMedium
                            background: Rectangle { radius: Theme.radius - 2; color: Theme.surface; border.color: Theme.border }
                        }
                        ListView {
                            id: fontList
                            objectName: "subFontList"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 240
                            clip: true
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar {}
                            model: fontPopup.fams.filter(function (f) {
                                return f.toLowerCase().indexOf(fontSearch.text.trim().toLowerCase()) >= 0 })
                            delegate: ItemDelegate {
                                required property string modelData
                                width: fontList.width - 10
                                height: 30
                                onClicked: { root.edit("font", modelData); fontPopup.close() }
                                background: Rectangle {
                                    radius: Theme.radius - 2
                                    color: hovered ? Theme.surfaceHover : "transparent"
                                }
                                contentItem: Text {
                                    text: modelData !== "" ? modelData : qsTr("System default")
                                    color: modelData === root.font ? Theme.accentHover : Theme.primaryText
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSizeMedium
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }
                            }
                        }
                    }
                }
            }
        }
        SwitchRow {
            name: "subBold"
            title: qsTr("Bold")
            keywords: "weight heavy thick"
            checked: root.bold
            onToggled: (v) => root.edit("bold", v)
        }
        StepperRow {
            name: "subShadow"
            title: qsTr("Shadow")
            description: qsTr("Offset of the soft shadow behind the text.")
            keywords: "drop depth"
            value: root.shadow; from: 0; to: 5; step: 0.5; decimals: 1
            onPicked: (v) => root.edit("shadow", v)
        }
        SwitchRow {
            name: "subBox"
            title: qsTr("Background box")
            description: qsTr("Draws a dark box behind each line instead of an outline.")
            keywords: "backdrop opaque black background"
            checked: root.box
            onToggled: (v) => root.edit("box", v)
        }
        SliderRow {
            name: "subBoxOpacity"
            title: qsTr("Box opacity")
            keywords: "transparency alpha background"
            value: root.boxOpacity; from: 10; to: 100; step: 5; suffix: "%"
            onMoved: (v) => root.edit("boxOpacity", v)
        }
        SettingRow {
            title: qsTr("Horizontal alignment")
            keywords: "left right center align"
            SegmentedControl {
                name: "subAlignX"
                accessibleName: qsTr("Horizontal alignment")
                model: [ { label: qsTr("Left"), value: "left" }, { label: qsTr("Center"), value: "center" },
                         { label: qsTr("Right"), value: "right" } ]
                value: root.alignX
                onPicked: (v) => root.edit("alignX", v)
            }
        }
        SettingRow {
            title: qsTr("Vertical alignment")
            description: qsTr("Anchor for the vertical position above.")
            keywords: "top bottom middle align"
            SegmentedControl {
                name: "subAlignY"
                accessibleName: qsTr("Vertical alignment")
                model: [ { label: qsTr("Top"), value: "top" }, { label: qsTr("Center"), value: "center" },
                         { label: qsTr("Bottom"), value: "bottom" } ]
                value: root.alignY
                onPicked: (v) => root.edit("alignY", v)
            }
        }
        SwitchRow {
            name: "subAssOverride"
            title: qsTr("Override styled subtitles")
            description: qsTr("Applies your style to ASS/SSA subtitles instead of their built-in look.")
            keywords: "ass ssa force styles karaoke"
            checked: root.assOverride
            onToggled: (v) => root.edit("assOverride", v)
        }
    }

    // ------------------------------------------------------------------ Encoding
    SettingSection {
        title: qsTr("Text encoding")
        ComboRow {
            name: "subCodepage"
            title: qsTr("Encoding")
            description: qsTr("Auto-detect handles most files. Choose one if letters look garbled.")
            keywords: "codepage charset utf latin garbled unicode windows"
            model: root.encodings
            value: root.codepage
            onPicked: (v) => root.edit("codepage", v)
        }
    }
}
