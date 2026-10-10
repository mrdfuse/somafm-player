import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.somafm

QQC2.ItemDelegate {
    id: delegate

    required property int index
    required property string channelId
    required property string title
    required property string description
    required property string genre
    required property string image
    required property int listeners
    required property string lastPlaying
    required property bool favorite

    readonly property string stationId: channelId
    readonly property bool isActive: Player.channelId === channelId
    readonly property bool isPlaying: isActive && Player.playing

    width: ListView.view.width
    highlighted: ListView.isCurrentItem || (applicationWindow().pageStack.depth > 1
                                            && applicationWindow().pageStack.get(1).channelId === channelId)

    onClicked: {
        ListView.view.currentIndex = index
        applicationWindow().showChannel(channelId)
    }
    onDoubleClicked: Player.play(channelId)

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing

        Item {
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4
            Layout.preferredHeight: Kirigami.Units.gridUnit * 4
            Layout.alignment: Qt.AlignTop

            Image {
                anchors.fill: parent
                source: delegate.image
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                smooth: true
                sourceSize.width: width * Screen.devicePixelRatio
                sourceSize.height: height * Screen.devicePixelRatio
            }
            Rectangle {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                width: Kirigami.Units.iconSizes.smallMedium + Kirigami.Units.smallSpacing
                height: width
                radius: width / 2
                color: Kirigami.Theme.highlightColor
                visible: delegate.isPlaying
                Kirigami.Icon {
                    anchors.centerIn: parent
                    width: Kirigami.Units.iconSizes.small
                    height: width
                    source: "audio-volume-high"
                    color: Kirigami.Theme.highlightedTextColor
                    isMask: true
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            Kirigami.Heading {
                Layout.fillWidth: true
                level: 4
                text: delegate.title
                elide: Text.ElideRight
                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: delegate.description
                wrapMode: Text.WordWrap
                maximumLineCount: 2
                elide: Text.ElideRight
                opacity: 0.85
                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.smallSpacing
                visible: delegate.lastPlaying.length > 0
                spacing: Kirigami.Units.smallSpacing
                Kirigami.Icon {
                    Layout.preferredWidth: Kirigami.Units.iconSizes.small
                    Layout.preferredHeight: Kirigami.Units.iconSizes.small
                    source: "media-playback-start"
                    opacity: 0.7
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    text: delegate.isActive && Player.title
                          ? (Player.artist ? Player.artist + " - " : "") + Player.title
                          : delegate.lastPlaying
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
                }
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: delegate.genre + "  ·  " + (delegate.listeners === 1 ? qsTr("1 listener") : qsTr("%1 listeners").arg(delegate.listeners))
                elide: Text.ElideRight
                font: Kirigami.Theme.smallFont
                color: delegate.highlighted ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.disabledTextColor
            }
        }

        QQC2.ToolButton {
            icon.name: delegate.isPlaying ? "media-playback-stop" : "media-playback-start"
            text: delegate.isPlaying ? qsTr("Stop") : qsTr("Play")
            display: QQC2.AbstractButton.IconOnly
            onClicked: delegate.isPlaying ? Player.stop() : Player.play(delegate.channelId)
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        QQC2.ToolButton {
            icon.name: delegate.favorite ? "starred-symbolic" : "non-starred-symbolic"
            icon.color: delegate.favorite ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor
            text: delegate.favorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
            display: QQC2.AbstractButton.IconOnly
            onClicked: Soma.toggleFavorite(delegate.channelId)
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
    }
}
