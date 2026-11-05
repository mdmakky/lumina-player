import QtQuick
import QtQuick.Layouts
import Lumina.Player

// One settings page: sections plus a "reset this page" button. While searching,
// every page with matches is shown (headed by its title) instead of just the current one.
ColumnLayout {
    id: page

    property string title
    property int pageIndex: 0
    property bool resettable: true
    property string resetLabel: qsTr("Reset this page to defaults")
    signal resetRequested()
    default property alias sections: body.data

    readonly property bool matched: {
        body.children.length
        for (let i = 0; i < body.children.length; ++i)
            if (body.children[i].matched === true) return true
        return false
    }
    readonly property bool searching: SettingsState.q !== ""

    visible: searching ? matched : SettingsState.page === pageIndex
    Layout.fillWidth: true
    spacing: Theme.spacingLarge

    Text {
        visible: page.searching
        text: page.title
        color: Theme.primaryText
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeLarge
        font.weight: Font.DemiBold
    }
    ColumnLayout {
        id: body
        Layout.fillWidth: true
        spacing: Theme.spacingLarge
    }
    SettingsButton {
        objectName: "settingsReset_" + page.pageIndex
        visible: page.resettable && !page.searching
        Layout.alignment: Qt.AlignRight
        text: page.resetLabel
        onClicked: page.resetRequested()
    }
}
