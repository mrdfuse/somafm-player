import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.somafm

Kirigami.ScrollablePage {
    id: page

    property string channelId
    property var ch: Soma.channel(channelId)
    property var songs: []

    readonly property bool isActive: Player.channelId === channelId
    readonly property bool isPlaying: isActive && Player.playing
    readonly property bool isFavorite: Soma.favorites.indexOf(channelId) >= 0

    title: ch.title || ""

    function reload() {
        ch = Soma.channel(channelId)
        Soma.fetchSongs(channelId)
    }

    onChannelIdChanged: {
        songs = []
        reload()
    }
    Component.onCompleted: reload()

    Connections {
        target: Soma
        function onChannelsUpdated() { page.ch = Soma.channel(page.channelId) }
        function onSongsReceived(id, list) {
            if (id === page.channelId)
                page.songs = list
        }
    }

    // Keep the "recently played" list current, like the website does.
    Timer {
        interval: 20000
        repeat: true
        running: page.visible && !page.isActive
        onTriggered: Soma.fetchSongs(page.channelId)
    }

    function formatTime(secs) {
        return new Date(secs * 1000).toLocaleTimeString(Qt.locale(), Locale.ShortFormat)
    }

    actions: [
        Kirigami.Action {
            text: qsTr("Open on somafm.com")
            icon.name: "internet-web-browser"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Qt.openUrlExternally("https://somafm.com/" + page.channelId + "/")
        },
        Kirigami.Action {
            text: qsTr("Full playlist history")
            icon.name: "view-media-playlist"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Qt.openUrlExternally("https://somafm.com/" + page.channelId + "/songhistory.html")
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        // Station header
        GridLayout {
            Layout.fillWidth: true
            columns: page.width > Kirigami.Units.gridUnit * 26 ? 2 : 1
            columnSpacing: Kirigami.Units.largeSpacing * 2
            rowSpacing: Kirigami.Units.largeSpacing

            Image {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 10
                Layout.preferredHeight: Kirigami.Units.gridUnit * 10
                Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
                source: page.ch.xlImage || ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                sourceSize.width: width * Screen.devicePixelRatio
                sourceSize.height: height * Screen.devicePixelRatio
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: Kirigami.Units.smallSpacing

                Kirigami.Heading {
                    Layout.fillWidth: true
                    text: page.ch.title || ""
                    wrapMode: Text.WordWrap
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: page.ch.description || ""
                    wrapMode: Text.WordWrap
                }

                GridLayout {
                    Layout.topMargin: Kirigami.Units.smallSpacing
                    columns: 2
                    columnSpacing: Kirigami.Units.largeSpacing
                    rowSpacing: Kirigami.Units.smallSpacing

                    QQC2.Label { text: qsTr("Genre:"); color: Kirigami.Theme.disabledTextColor }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: page.ch.genre || ""
                        wrapMode: Text.WordWrap
                    }
                    QQC2.Label { text: qsTr("DJ:"); color: Kirigami.Theme.disabledTextColor }
                    QQC2.Label {
                        Layout.fillWidth: true
                        textFormat: Text.StyledText
                        text: page.ch.djmail
                              ? "<a href=\"mailto:" + page.ch.djmail + "\">" + page.ch.dj + "</a>"
                              : (page.ch.dj || "")
                        onLinkActivated: link => Qt.openUrlExternally(link)
                        wrapMode: Text.WordWrap
                        HoverHandler { cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor }
                    }
                    QQC2.Label { text: qsTr("Listeners:"); color: Kirigami.Theme.disabledTextColor }
                    QQC2.Label { text: (page.ch.listeners || 0).toLocaleString(Qt.locale(), "f", 0) }
                }

                RowLayout {
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    QQC2.Button {
                        text: page.isPlaying ? qsTr("Stop") : qsTr("Play")
                        icon.name: page.isPlaying ? "media-playback-stop" : "media-playback-start"
                        highlighted: !page.isPlaying
                        onClicked: page.isPlaying ? Player.stop() : Player.play(page.channelId)
                    }
                    QQC2.Button {
                        text: page.isFavorite ? qsTr("Favorite") : qsTr("Add to favorites")
                        icon.name: page.isFavorite ? "starred-symbolic" : "non-starred-symbolic"
                        icon.color: page.isFavorite ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor
                        checkable: true
                        checked: page.isFavorite
                        onClicked: Soma.toggleFavorite(page.channelId)
                    }
                }
            }
        }

        // Now playing
        Kirigami.AbstractCard {
            id: nowCard
            Layout.fillWidth: true
            visible: page.songs.length > 0 || (page.ch.lastPlaying || "").length > 0

            readonly property var current: page.songs.length > 0 ? page.songs[0] : null

            contentItem: RowLayout {
                spacing: Kirigami.Units.largeSpacing
                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.large
                    Layout.preferredHeight: Kirigami.Units.iconSizes.large
                    source: page.isPlaying ? "audio-volume-high" : "media-playback-start"
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    QQC2.Label {
                        text: page.isPlaying ? qsTr("Now playing") : qsTr("On air now")
                        font: Kirigami.Theme.smallFont
                        color: Kirigami.Theme.disabledTextColor
                    }
                    Kirigami.Heading {
                        Layout.fillWidth: true
                        level: 3
                        text: nowCard.current ? nowCard.current.title : (page.ch.lastPlaying || "")
                        wrapMode: Text.WordWrap
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        visible: text.length > 0
                        text: {
                            const s = nowCard.current
                            if (!s) return ""
                            return s.artist + (s.album ? "  ·  " + s.album : "")
                        }
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.largeSpacing
            level: 3
            text: qsTr("Recently played")
            visible: page.songs.length > 1
        }

        Repeater {
            model: page.songs.length > 1 ? page.songs.slice(1) : []

            delegate: QQC2.ItemDelegate {
                id: songRow
                required property var modelData
                Layout.fillWidth: true

                onClicked: songMenu.popup()

                contentItem: RowLayout {
                    spacing: Kirigami.Units.largeSpacing
                    QQC2.Label {
                        Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                        Layout.alignment: Qt.AlignTop
                        text: page.formatTime(songRow.modelData.date)
                        color: Kirigami.Theme.disabledTextColor
                        font.features: { "tnum": 1 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: songRow.modelData.title
                            font.bold: true
                            elide: Text.ElideRight
                        }
                        QQC2.Label {
                            Layout.fillWidth: true
                            text: songRow.modelData.artist
                                  + (songRow.modelData.album ? "  ·  " + songRow.modelData.album : "")
                            elide: Text.ElideRight
                            opacity: 0.8
                        }
                    }
                }

                QQC2.Menu {
                    id: songMenu
                    QQC2.MenuItem {
                        text: qsTr("Copy “Artist – Title”")
                        icon.name: "edit-copy"
                        onTriggered: Soma.copyToClipboard(songRow.modelData.artist + " - " + songRow.modelData.title)
                    }
                    QQC2.MenuItem {
                        text: qsTr("Search on the web")
                        icon.name: "edit-find"
                        onTriggered: Qt.openUrlExternally("https://duckduckgo.com/?q="
                            + encodeURIComponent(songRow.modelData.artist + " " + songRow.modelData.title))
                    }
                }
            }
        }

        QQC2.BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            visible: page.songs.length === 0
            running: visible
        }
    }
}
