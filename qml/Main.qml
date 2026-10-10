import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.somafm

Kirigami.ApplicationWindow {
    id: root

    title: Player.playing && Player.title
           ? (Player.artist ? Player.artist + " – " : "") + Player.title
           : ""

    width: Kirigami.Units.gridUnit * 60
    height: Kirigami.Units.gridUnit * 40
    minimumWidth: Kirigami.Units.gridUnit * 22
    minimumHeight: Kirigami.Units.gridUnit * 20

    pageStack.defaultColumnWidth: Kirigami.Units.gridUnit * 28
    pageStack.globalToolBar.style: Kirigami.ApplicationHeaderStyle.ToolBar
    pageStack.globalToolBar.showNavigationButtons: Kirigami.ApplicationHeaderStyle.ShowBackButton
    pageStack.initialPage: ChannelListPage {}

    footer: PlayerBar {}

    // Opens (or reuses) the detail column for a station.
    function showChannel(id) {
        if (pageStack.depth > 1) {
            pageStack.get(1).channelId = id
            pageStack.currentIndex = 1
        } else {
            pageStack.push(Qt.resolvedUrl("ChannelPage.qml"), { channelId: id })
        }
    }

    Component.onCompleted: {
        lastPlayerChannel = Player.channelId
        if (Player.channelId)
            showChannel(Player.channelId)
    }

    // When the detail column shows the playing station, let it follow
    // station changes from next/previous or the media keys.
    property string lastPlayerChannel

    Connections {
        target: Player
        function onChannelChanged() {
            if (pageStack.depth <= 1 && Player.channelId)
                root.showChannel(Player.channelId)
            else if (pageStack.depth > 1 && pageStack.get(1).channelId === root.lastPlayerChannel)
                pageStack.get(1).channelId = Player.channelId
            root.lastPlayerChannel = Player.channelId
        }
        function onRaiseRequested() {
            root.show()
            root.raise()
            root.requestActivate()
        }
    }

    Shortcut {
        sequences: ["Media Play", "Media Pause", "Media Toggle Play", "Ctrl+Space"]
        onActivated: Player.toggle()
    }
    Shortcut {
        sequences: ["Media Next", "Ctrl+Right"]
        onActivated: Player.next()
    }
    Shortcut {
        sequences: ["Media Previous", "Ctrl+Left"]
        onActivated: Player.previous()
    }
    Shortcut {
        sequence: StandardKey.Find
        onActivated: pageStack.get(0).focusSearch()
    }
    Shortcut {
        sequence: StandardKey.Quit
        onActivated: Qt.quit()
    }
}
