import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import Lumina.Player

// "Picture": zoom/pan/crop/flip, sharpen/denoise, scaling, HDR tone mapping, interpolation, custom shader.
// Everything is live through engine.setPicture; scaling/HDR/interpolation also become the saved defaults.
PopupPanel {
    id: popup

    padding: Theme.spacingLarge
    panelWidth: 340

    readonly property var pic: player.engine.picture

    function setGlobal(key, v) { // engine + Settings > Video default
        player.engine.setPicture(key, v)
        player.setSetting("video/" + key, v)
    }
    function resetAll() {
        player.engine.resetPicture()
        for (const k of ["scale", "tonemap", "interp"]) player.setSetting("video/" + k, player.engine.picture[k])
    }

    // Row of mutually exclusive pills. options: [{label, value}]
    component ChipRow: Flow {
        id: cr
        property var options: []
        property var current
        property string prefix
        signal picked(var v)
        Layout.fillWidth: true
        Layout.leftMargin: 4
        spacing: 6
        Repeater {
            model: cr.options
            delegate: MenuChip {
                required property var modelData
                objectName: cr.prefix + "_" + modelData.value
                text: modelData.label
                closeOnClick: false
                selected: String(cr.current) === String(modelData.value)
                onTriggered: cr.picked(modelData.value)
            }
        }
    }

    component PicSlider: RowLayout {
        id: ps
        property string label
        property alias from: sl.from
        property alias to: sl.to
        property alias value: sl.value
        property alias sliderName: sl.objectName
        property string display
        signal moved(real v)
        Layout.fillWidth: true
        Layout.leftMargin: 4
        spacing: Theme.spacingMedium
        Text {
            Layout.preferredWidth: 50
            text: ps.label
            color: Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
        }
        Slider {
            id: sl
            Layout.fillWidth: true
            stepSize: 0.01
            onMoved: ps.moved(value)
            background: Rectangle {
                x: sl.leftPadding
                y: sl.topPadding + sl.availableHeight / 2 - height / 2
                width: sl.availableWidth
                height: 3
                radius: 2
                color: Theme.trackIdle
                Rectangle {
                    readonly property real zeroPos: (0 - sl.from) / (sl.to - sl.from)
                    x: Math.min(parent.width * zeroPos, sl.visualPosition * parent.width)
                    width: Math.abs(sl.visualPosition - zeroPos) * parent.width
                    height: parent.height
                    color: Theme.accent
                }
            }
            handle: Rectangle {
                x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
                y: sl.topPadding + sl.availableHeight / 2 - height / 2
                width: 12; height: 12; radius: 6
                color: Theme.primaryText
            }
        }
        Text {
            Layout.preferredWidth: 46
            horizontalAlignment: Text.AlignRight
            text: ps.display
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    readonly property var levels: [
        { label: qsTr("Off"), value: 0 }, { label: qsTr("Low"), value: 1 },
        { label: qsTr("Medium"), value: 2 }, { label: qsTr("High"), value: 3 }
    ]

    MenuSectionHeader {
        text: qsTr("Zoom & pan")
        actionText: qsTr("Reset")
        actionObjectName: "pictureZoomReset"
        actionEnabled: pic.zoom !== 0 || pic.panx !== 0 || pic.pany !== 0
        onActionClicked: { player.engine.setPicture("zoom", 0); player.engine.setPicture("panx", 0); player.engine.setPicture("pany", 0) }
    }
    PicSlider {
        label: qsTr("Zoom"); sliderName: "pictureZoom"; from: -2; to: 3
        value: pic.zoom; display: Math.round(Math.pow(2, pic.zoom) * 100) + "%"
        onMoved: (v) => player.engine.setPicture("zoom", Math.abs(v) < 0.04 ? 0 : v) // snaps to 100%
    }
    PicSlider {
        label: qsTr("Pan X"); sliderName: "picturePanX"; from: -1; to: 1
        value: pic.panx; display: Math.round(pic.panx * 100) + "%"
        onMoved: (v) => player.engine.setPicture("panx", v)
    }
    PicSlider {
        label: qsTr("Pan Y"); sliderName: "picturePanY"; from: -1; to: 1
        value: pic.pany; display: Math.round(pic.pany * 100) + "%"
        onMoved: (v) => player.engine.setPicture("pany", v)
    }
    Text {
        Layout.leftMargin: 4
        text: qsTr("Shift + drag the video to pan.")
        color: Theme.secondaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeSmall
    }

    MenuSectionHeader { text: qsTr("Crop") }
    ChipRow {
        prefix: "pictureCrop"; current: pic.crop
        options: [{ label: qsTr("Off"), value: "off" }, { label: "16:9", value: "16:9" }, { label: "4:3", value: "4:3" },
                  { label: "21:9", value: "21:9" }, { label: "1:1", value: "1:1" }]
        onPicked: (v) => player.engine.setPicture("crop", v)
    }

    MenuSectionHeader { text: qsTr("Flip") }
    Flow {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        spacing: 6
        MenuChip {
            objectName: "pictureFlip_h"
            text: qsTr("Horizontal")
            closeOnClick: false
            selected: pic.hflip
            onTriggered: player.engine.setPicture("hflip", !pic.hflip)
        }
        MenuChip {
            objectName: "pictureFlip_v"
            text: qsTr("Vertical")
            closeOnClick: false
            selected: pic.vflip
            onTriggered: player.engine.setPicture("vflip", !pic.vflip)
        }
    }

    MenuSectionHeader { text: qsTr("Sharpen") }
    ChipRow {
        prefix: "pictureSharpen"; current: pic.sharpen; options: popup.levels
        onPicked: (v) => player.engine.setPicture("sharpen", v)
    }
    MenuSectionHeader { text: qsTr("Denoise") }
    ChipRow {
        prefix: "pictureDenoise"; current: pic.denoise; options: popup.levels
        onPicked: (v) => player.engine.setPicture("denoise", v)
    }

    MenuSectionHeader { text: qsTr("Scaling quality") }
    ChipRow {
        prefix: "pictureScale"; current: pic.scale
        options: [{ label: qsTr("Fast"), value: "fast" }, { label: qsTr("Balanced"), value: "balanced" },
                  { label: qsTr("High quality"), value: "high" }]
        onPicked: (v) => popup.setGlobal("scale", v)
    }

    MenuSectionHeader { text: qsTr("HDR tone mapping") }
    ChipRow {
        prefix: "pictureTone"; current: pic.tonemap
        options: [{ label: qsTr("Auto"), value: "auto" }, { label: "BT.2390", value: "bt.2390" },
                  { label: "Reinhard", value: "reinhard" }, { label: "Hable", value: "hable" }]
        onPicked: (v) => popup.setGlobal("tonemap", v)
    }

    MenuSectionHeader { text: qsTr("Smooth motion") }
    ChipRow {
        prefix: "pictureInterp"; current: pic.interp
        options: [{ label: qsTr("Off"), value: false }, { label: qsTr("On"), value: true }]
        onPicked: (v) => popup.setGlobal("interp", v)
    }
    Text {
        Layout.leftMargin: 4
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: qsTr("Frame interpolation blends frames to match your display. It uses more GPU.")
        color: Theme.secondaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeSmall
    }

    MenuSectionHeader {
        text: qsTr("Custom shader")
        actionText: qsTr("Clear")
        actionObjectName: "pictureShaderClear"
        actionEnabled: pic.shader !== ""
        onActionClicked: player.engine.setPicture("shader", "")
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 4
        spacing: Theme.spacingMedium
        Text {
            Layout.fillWidth: true
            elide: Text.ElideMiddle
            text: pic.shader !== "" ? pic.shader.split("/").pop() : qsTr("None")
            color: pic.shader !== "" ? Theme.primaryText : Theme.secondaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
        }
        Button {
            objectName: "pictureShaderPick"
            text: qsTr("Choose…")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMedium
            onClicked: shaderDialog.open()
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
    FileDialog {
        id: shaderDialog
        title: qsTr("Choose a GLSL shader")
        nameFilters: [qsTr("GLSL shaders (*.glsl *.hook *.frag)"), qsTr("All files (*)")]
        onAccepted: player.engine.setPicture("shader",
            decodeURIComponent(selectedFile.toString().replace(/^file:\/\/(\/(?=[A-Za-z]:))?/, "")))
    }

    Button {
        objectName: "pictureReset"
        Layout.alignment: Qt.AlignRight
        Layout.topMargin: Theme.spacingSmall
        text: qsTr("Reset picture")
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMedium
        onClicked: popup.resetAll()
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
