pragma Singleton
import QtQuick
import Lumina.Player

// Shared state of the settings dialog: current page, global search query and
// the scroll-into-view request raised by focused rows.
QtObject {
    property int page: 0
    property string query: ""
    readonly property string q: query.trim().toLowerCase()
    // Card/sidebar surface: one step away from the dialog panel in every theme.
    readonly property color card: Theme.pick(Theme.surface, Theme.background, Theme.surface)
    property int resetTick: 0   // bumped by a page reset so non-reactive rows re-read their settings
    signal reveal(Item item)
}
