import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Lumina.Player

// Media information / codec inspector: file, container, per-stream details, decoder and
// renderer. Data comes from mpv properties through the engine; "Copy as text" puts the
// same report on the clipboard.
Popup {
    id: dlg

    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(680, (parent ? parent.width : 680) - 48)
    height: Math.min(640, (parent ? parent.height : 640) - 48)
    padding: 0
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    // [{ title, blocks: [{ head, rows: [[key, value], ...] }] }]
    property var sections: []
    readonly property string reportText: buildText()

    function prop(name) { return player.engine.mpvProperty(name) }
    function has(v) { return v !== undefined && v !== null && v !== "" }
    function bitrate(bps) {
        if (!has(bps) || bps <= 0) return ""
        return bps >= 1e6 ? (bps / 1e6).toFixed(2) + " Mb/s" : Math.round(bps / 1000) + " kb/s"
    }
    function bytes(n) {
        if (!has(n) || n <= 0) return ""
        const u = ["B", "KiB", "MiB", "GiB", "TiB"]
        let i = 0
        while (n >= 1024 && i < u.length - 1) { n /= 1024; ++i }
        return (i === 0 ? n : n.toFixed(2)) + " " + u[i]
    }
    function yesNo(b) { return b ? qsTr("Yes") : qsTr("No") }
    // Appends [key, value] when the value is known.
    function add(rows, key, value) { if (has(value) && value !== 0) rows.push([key, String(value)]) }

    function collect() {
        const secs = []
        const path = prop("path") || ""
        const dur = prop("duration")
        const size = prop("file-size")
        const general = []
        add(general, qsTr("File"), path)
        add(general, qsTr("Size"), bytes(size))
        add(general, qsTr("Container"), prop("file-format"))
        add(general, qsTr("Duration"), has(dur) ? timefmt.formatDuration(dur) : "")
        if (has(size) && has(dur) && dur > 0 && size > 0)
            add(general, qsTr("Overall bitrate"), bitrate(size * 8 / dur))
        const md = prop("metadata")
        if (md) {
            add(general, qsTr("Title"), md["title"] || md["TITLE"])
            add(general, qsTr("Artist"), md["artist"] || md["ARTIST"])
        }
        const chapters = prop("chapter-list")
        add(general, qsTr("Chapters"), chapters ? chapters.length : 0)
        secs.push({ title: qsTr("GENERAL"), blocks: [{ head: "", rows: general }] })

        const tracks = prop("track-list") || []
        const vp = prop("video-params") || {}
        const ap = prop("audio-params") || {}
        const vBlocks = [], aBlocks = [], sBlocks = []
        let vn = 0, an = 0, sn = 0
        for (const t of tracks) {
            const rows = []
            if (t.type === "video" && !t.albumart) {
                const cur = t.selected && has(vp.w)
                const w = cur ? vp.w : t["demux-w"], h = cur ? vp.h : t["demux-h"]
                add(rows, qsTr("Codec"), t.codec)
                add(rows, qsTr("Profile"), t["codec-profile"])
                add(rows, qsTr("Resolution"), has(w) ? w + " × " + h : "")
                add(rows, qsTr("Frame rate"), has(t["demux-fps"]) ? Number(t["demux-fps"]).toFixed(3) + " fps" : "")
                if (cur) {
                    add(rows, qsTr("Pixel format"), vp.pixelformat)
                    let depth = vp["plane-depth"]
                    if (!has(depth)) {
                        const m = /^nv\d+$/.test(vp.pixelformat || "") ? null : /(10|12|14|16)(le|be)?$/.exec(vp.pixelformat || "")
                        depth = m ? m[1] : 8
                    }
                    add(rows, qsTr("Bit depth"), depth + " bit")
                    add(rows, qsTr("Colour matrix"), vp.colormatrix)
                    add(rows, qsTr("Colour range"), vp.colorlevels)
                    add(rows, qsTr("Primaries"), vp.primaries)
                    add(rows, qsTr("Transfer"), vp.gamma)
                    const hdr = vp.gamma === "pq" ? qsTr("HDR10 / PQ") : vp.gamma === "hlg" ? qsTr("HLG")
                              : (has(vp["sig-peak"]) && vp["sig-peak"] > 1.01 ? qsTr("HDR") : qsTr("SDR"))
                    add(rows, qsTr("Dynamic range"), hdr)
                }
                add(rows, qsTr("Bitrate"), bitrate(t["demux-bitrate"]))
                add(rows, qsTr("Language"), t.lang)
                add(rows, qsTr("Title"), t.title)
                vBlocks.push({ head: qsTr("Video %1").arg(++vn) + (t.selected ? "" : ""), rows: rows })
            } else if (t.type === "audio") {
                const cur = t.selected && has(ap.samplerate)
                add(rows, qsTr("Codec"), t.codec)
                add(rows, qsTr("Channels"), t["demux-channel-count"])
                add(rows, qsTr("Layout"), cur ? ap["hr-channels"] || ap.channels : t["demux-channels"])
                add(rows, qsTr("Sample rate"), has(cur ? ap.samplerate : t["demux-samplerate"])
                    ? (cur ? ap.samplerate : t["demux-samplerate"]) + " Hz" : "")
                add(rows, qsTr("Bitrate"), bitrate(t["demux-bitrate"]))
                add(rows, qsTr("Language"), t.lang)
                add(rows, qsTr("Title"), t.title)
                add(rows, qsTr("Default"), t["default"] ? qsTr("Yes") : "")
                aBlocks.push({ head: qsTr("Audio %1").arg(++an), rows: rows })
            } else if (t.type === "sub") {
                add(rows, qsTr("Format"), t.codec)
                add(rows, qsTr("Language"), t.lang)
                add(rows, qsTr("Title"), t.title)
                rows.push([qsTr("Default"), yesNo(t["default"])])
                rows.push([qsTr("Forced"), yesNo(t.forced)])
                if (t.external) rows.push([qsTr("External"), qsTr("Yes")])
                sBlocks.push({ head: qsTr("Subtitle %1").arg(++sn), rows: rows })
            }
        }
        if (vBlocks.length) secs.push({ title: qsTr("VIDEO"), blocks: vBlocks })
        if (aBlocks.length) secs.push({ title: qsTr("AUDIO"), blocks: aBlocks })
        if (sBlocks.length) secs.push({ title: qsTr("SUBTITLES"), blocks: sBlocks })

        const about = player.aboutInfo()
        const play = []
        add(play, qsTr("Decoder"), about.decoder || qsTr("software"))
        add(play, qsTr("Renderer"), about.renderer)
        secs.push({ title: qsTr("PLAYBACK"), blocks: [{ head: "", rows: play }] })
        return secs
    }

    function buildText() {
        let out = ""
        for (const s of sections) {
            out += "== " + s.title + " ==\n"
            for (const b of s.blocks) {
                if (b.head) out += "[" + b.head + "]\n"
                for (const r of b.rows) out += r[0] + ": " + r[1] + "\n"
            }
            out += "\n"
        }
        return out.trim() + "\n"
    }

    onAboutToShow: sections = collect()

    Overlay.modal: Rectangle { color: Theme.scrimModal }
    background: Rectangle {
        color: Theme.surface
        border.color: Theme.border
        radius: Theme.radiusLarge
    }

    contentItem: ColumnLayout {
        spacing: 0

        Text {
            Layout.margins: Theme.spacingLarge
            Layout.bottomMargin: Theme.spacingSmall
            text: qsTr("Media information")
            color: Theme.primaryText
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeHeading
            font.weight: Font.DemiBold
            Accessible.role: Accessible.Heading
        }

        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.spacingLarge
            Layout.rightMargin: Theme.spacingSmall
            clip: true
            contentWidth: width
            contentHeight: body.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            ColumnLayout {
                id: body
                width: flick.width - Theme.spacingLarge
                spacing: Theme.spacingSmall

                Repeater {
                    model: dlg.sections
                    delegate: ColumnLayout {
                        id: sec
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: 4

                        Text {
                            Layout.topMargin: Theme.spacingMedium
                            text: sec.modelData.title
                            color: Theme.disabledText
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeSmall
                            font.letterSpacing: 2
                        }
                        Repeater {
                            model: sec.modelData.blocks
                            delegate: Rectangle {
                                id: blk
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: blockCol.implicitHeight + 2 * Theme.spacingMedium
                                radius: Theme.radius
                                color: Theme.elevated
                                border.color: Theme.border

                                ColumnLayout {
                                    id: blockCol
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingMedium
                                    spacing: 3
                                    Text {
                                        visible: blk.modelData.head !== ""
                                        text: blk.modelData.head
                                        color: Theme.accentHover
                                        font.family: Theme.fontFamily
                                        font.pixelSize: Theme.fontSizeMedium
                                        font.weight: Font.Medium
                                    }
                                    Repeater {
                                        model: blk.modelData.rows
                                        delegate: RowLayout {
                                            id: line
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: Theme.spacingMedium
                                            Text {
                                                Layout.preferredWidth: 130
                                                Layout.alignment: Qt.AlignTop
                                                text: line.modelData[0]
                                                color: Theme.secondaryText
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.fontSizeMedium
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: line.modelData[1]
                                                wrapMode: Text.WrapAnywhere
                                                color: Theme.primaryText
                                                font.family: Theme.fontFamily
                                                font.pixelSize: Theme.fontSizeMedium
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.margins: Theme.spacingLarge
            Layout.topMargin: Theme.spacingMedium
            Item { Layout.fillWidth: true }
            SettingsButton {
                objectName: "mediaInfoCopyButton"
                text: qsTr("Copy as text")
                onClicked: { player.copyText(dlg.reportText); player.toast(qsTr("Media information copied")) }
            }
            SettingsButton {
                objectName: "mediaInfoCloseButton"
                text: qsTr("Close")
                primary: true
                onClicked: dlg.close()
            }
        }
    }
}
