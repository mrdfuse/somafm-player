import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.somafm

Kirigami.ScrollablePage {
    id: page

    title: "SomaFM"

    function focusSearch() {
        search.forceActiveFocus()
        search.selectAll()
    }

    actions: [
        Kirigami.Action {
            text: qsTr("Refresh")
            icon.name: "view-refresh"
            displayHint: Kirigami.DisplayHint.IconOnly
            enabled: !Soma.loading
            onTriggered: Soma.refresh()
        },
        Kirigami.Action {
            text: qsTr("Open somafm.com")
            icon.name: "internet-web-browser"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Qt.openUrlExternally("https://somafm.com/")
        },
        Kirigami.Action {
            text: qsTr("Support SomaFM")
            icon.name: "help-donate"
            displayHint: Kirigami.DisplayHint.AlwaysHide
            onTriggered: Qt.openUrlExternally("https://somafm.com/support/")
        }
    ]

    header: QQC2.ToolBar {
        contentItem: ColumnLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.TabBar {
                id: tabs
                Layout.fillWidth: true
                currentIndex: Soma.channels.favoritesOnly ? 1 : 0
                onCurrentIndexChanged: Soma.channels.favoritesOnly = currentIndex === 1

                QQC2.TabButton {
                    text: qsTr("All stations")
                    icon.name: "radio"
                }
                QQC2.TabButton {
                    text: Soma.favorites.length > 0
                          ? qsTr("Favorites (%1)").arg(Soma.favorites.length)
                          : qsTr("Favorites")
                    icon.name: "starred-symbolic"
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Kirigami.SearchField {
                    id: search
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search stations, genres, tracks…")
                    onTextChanged: Soma.channels.filterText = text
                    Keys.onDownPressed: { list.forceActiveFocus(); list.currentIndex = 0 }
                }

                QQC2.ComboBox {
                    id: genreBox
                    model: [qsTr("All genres")].concat(Soma.genres)
                    onActivated: index => Soma.channels.genre = index === 0 ? "" : Soma.genres[index - 1]
                    displayText: currentIndex <= 0 ? qsTr("All genres")
                                                   : currentText.charAt(0).toUpperCase() + currentText.slice(1)
                }

                QQC2.ComboBox {
                    model: [qsTr("Name"), qsTr("Listeners"), qsTr("Genre")]
                    currentIndex: Soma.channels.sortMode
                    onActivated: index => Soma.channels.sortMode = index
                    QQC2.ToolTip.text: qsTr("Sort by")
                    QQC2.ToolTip.visible: hovered
                    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                }
            }
        }
    }

    ListView {
        id: list
        model: Soma.channels
        currentIndex: -1
        keyNavigationEnabled: true

        delegate: ChannelDelegate {}

        Keys.onReturnPressed: if (currentItem) Player.play(currentItem.stationId)

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - Kirigami.Units.gridUnit * 4
            visible: list.count === 0
            icon.name: Soma.loading ? "radio" : Soma.channels.favoritesOnly ? "non-starred-symbolic" : "edit-find"
            text: Soma.loading ? qsTr("Loading stations…")
                 : Soma.error ? Soma.error
                 : Soma.channels.favoritesOnly && Soma.favorites.length === 0 ? qsTr("No favorites yet")
                 : qsTr("No stations found")
            explanation: !Soma.loading && Soma.channels.favoritesOnly && Soma.favorites.length === 0
                         ? qsTr("Click the star next to a station to add it here.") : ""
            helpfulAction: Soma.error ? retryAction : null

            Kirigami.Action {
                id: retryAction
                text: qsTr("Try again")
                icon.name: "view-refresh"
                onTriggered: Soma.refresh()
            }
        }
    }
}
