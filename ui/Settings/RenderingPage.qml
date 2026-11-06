import QtQuick
import QtQuick.Layouts
import Lumina.Player

// Settings > Rendering: decoder and renderer. `dlg` is the SettingsDialog (owner of hwdec state).
SettingsPage {
    id: page
    property var dlg
    title: qsTr("Rendering")
    resettable: false

    SettingSection {
        title: qsTr("Decoding")
        SettingRow {
            title: qsTr("Hardware decoding")
            description: qsTr("Same setting as Playback. Auto-safe uses only well-tested decoders; Off is the most compatible.")
            keywords: "hwdec gpu vaapi videotoolbox nvdec decoder acceleration"
            SegmentedControl {
                name: "hwdecOptionRendering"
                accessibleName: qsTr("Hardware decoding")
                model: [
                    { label: qsTr("Auto-safe"), value: "auto-safe" },
                    { label: qsTr("Off"), value: "no" },
                    { label: qsTr("Auto"), value: "auto" }
                ]
                value: page.dlg ? page.dlg.hwdec : "auto-safe"
                onPicked: (v) => page.dlg.setHwdec(v)
            }
        }
    }
    SettingSection {
        title: qsTr("Renderer")
        SettingRow {
            title: qsTr("Graphics API")
            description: qsTr("Video is drawn with mpv's OpenGL renderer inside the Qt Quick scene.")
            keywords: "opengl gl gpu renderer driver vsync"
            Text {
                objectName: "rendererInfo"
                Layout.maximumWidth: 360
                horizontalAlignment: Text.AlignRight
                wrapMode: Text.WordWrap
                text: page.dlg && page.dlg.about.renderer ? page.dlg.about.renderer : qsTr("Not available yet")
                color: Theme.secondaryText
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMedium
            }
        }
    }
}
