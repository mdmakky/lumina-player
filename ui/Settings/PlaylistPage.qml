import QtQuick
import Lumina.Player

// Settings > Playlist: queue behaviour. Shuffle/repeat write the same keys PlayerController
// restores at start, so they double as "start with" defaults.
SettingsPage {
    id: page
    property var dlg
    title: qsTr("Playlist")
    resettable: false

    property bool rememberQueue: false
    property string openMode: "replace"
    Component.onCompleted: {
        rememberQueue = player.setting("playlist/rememberQueue", false) === true
                        || player.setting("playlist/rememberQueue", false) === "true"
        openMode = player.setting("playlist/openMode", "replace")
    }

    SettingSection {
        title: qsTr("Order")
        SwitchRow {
            name: "playlistShuffle"
            title: qsTr("Shuffle")
            description: qsTr("Also the state the player starts in.")
            keywords: "random order default"
            checked: player.shuffle
            onToggled: (v) => player.shuffle = v
        }
        SettingRow {
            title: qsTr("Repeat")
            description: qsTr("Also the state the player starts in.")
            keywords: "loop one all default"
            SegmentedControl {
                name: "playlistRepeat"
                accessibleName: qsTr("Repeat")
                model: [
                    { label: qsTr("Off"), value: 0 },
                    { label: qsTr("All"), value: 1 },
                    { label: qsTr("One"), value: 2 }
                ]
                value: player.repeatMode
                onPicked: (v) => player.repeatMode = v
            }
        }
        SwitchRow {
            name: "playlistAutoNext"
            title: qsTr("Play next automatically")
            description: qsTr("Continue with the next item when a file ends.")
            keywords: "auto advance continue queue"
            checked: page.dlg ? page.dlg.autoNext : true
            onToggled: (v) => page.dlg.setAutoNext(v)
        }
    }
    SettingSection {
        title: qsTr("Queue")
        SwitchRow {
            name: "rememberQueueSwitch"
            title: qsTr("Remember queue")
            description: qsTr("Keep the queue when you quit and bring it back next time (it does not start playing).")
            keywords: "restore persist session playlist exit"
            checked: page.rememberQueue
            onToggled: (v) => { page.rememberQueue = v; player.setSetting("playlist/rememberQueue", v) }
        }
        SettingRow {
            title: qsTr("When opening files")
            description: qsTr("Replace the queue, or add the files to the end of it.")
            keywords: "append add clear replace open drop"
            SegmentedControl {
                name: "playlistOpenMode"
                accessibleName: qsTr("When opening files")
                model: [
                    { label: qsTr("Replace queue"), value: "replace", id: "replace" },
                    { label: qsTr("Add to queue"), value: "append", id: "append" }
                ]
                value: page.openMode
                onPicked: (v) => { page.openMode = v; player.setSetting("playlist/openMode", v) }
            }
        }
    }
}
