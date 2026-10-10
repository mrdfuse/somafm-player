import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.somafm

QQC2.ToolBar {
    id: bar

    position: QQC2.ToolBar.Footer
    readonly property bool hasChannel: Player.channelId !== ""
    readonly property bool wide: width > Kirigami.Units.gridUnit * 40
    readonly property bool isFavorite: Soma.favorites.indexOf(Player.channelId) >= 0

    contentItem: RowLayout {
        spacing: Kirigami.Units.smallSpacing

        // Station logo and track info; click to open the station.
        RowLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: Kirigami.Units.largeSpacing

            Image {
                Layout.preferredWidth: Kirigami.Units.gridUnit * 2.5
                Layout.preferredHeight: Kirigami.Units.gridUnit * 2.5
                source: Player.channel.image || ""
                visible: bar.hasChannel
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                sourceSize.width: width * Screen.devicePixelRatio
                sourceSize.height: height * Screen.devicePixelRatio
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                spacing: 0

                QQC2.Label {
                    Layout.fillWidth: true
                    font.bold: true
                    elide: Text.ElideRight
                    text: !bar.hasChannel ? qsTr("Pick a station to start listening")
                          : (Player.title || Player.channel.title || "")
                }
                QQC2.Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    font: Kirigami.Theme.smallFont
                    color: Player.error ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.disabledTextColor
                    visible: bar.hasChannel
                    text: {
                        if (Player.error)
                            return Player.error
                        const parts = []
                        if (Player.artist) parts.push(Player.artist)
                        if (Player.channel.title && Player.title) parts.push(Player.channel.title)
                        if (Player.buffering) parts.push(qsTr("Buffering…"))
                        else if (!Player.playing) parts.push(qsTr("Stopped"))
                        return parts.join("  ·  ")
                    }
                }
            }

            TapHandler {
                enabled: bar.hasChannel
                onTapped: applicationWindow().showChannel(Player.channelId)
            }
            HoverHandler {
                cursorShape: bar.hasChannel ? Qt.PointingHandCursor : Qt.ArrowCursor
            }
        }

        QQC2.BusyIndicator {
            Layout.preferredWidth: Kirigami.Units.iconSizes.medium
            Layout.preferredHeight: Kirigami.Units.iconSizes.medium
            running: Player.buffering
            visible: running
        }

        QQC2.ToolButton {
            icon.name: bar.isFavorite ? "starred-symbolic" : "non-starred-symbolic"
            icon.color: bar.isFavorite ? Kirigami.Theme.neutralTextColor : Kirigami.Theme.textColor
            text: bar.isFavorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
            display: QQC2.AbstractButton.IconOnly
            visible: bar.hasChannel
            onClicked: Soma.toggleFavorite(Player.channelId)
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        Kirigami.Separator {
            Layout.fillHeight: true
            Layout.margins: Kirigami.Units.smallSpacing
            visible: bar.hasChannel
        }

        QQC2.ToolButton {
            icon.name: "media-skip-backward"
            text: Soma.favorites.length > 0 ? qsTr("Previous favorite") : qsTr("Previous station")
            display: QQC2.AbstractButton.IconOnly
            onClicked: Player.previous()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        QQC2.ToolButton {
            icon.name: Player.playing ? "media-playback-stop" : "media-playback-start"
            icon.width: Kirigami.Units.iconSizes.medium
            icon.height: Kirigami.Units.iconSizes.medium
            text: Player.playing ? qsTr("Stop") : qsTr("Play")
            display: QQC2.AbstractButton.IconOnly
            enabled: bar.hasChannel
            onClicked: Player.toggle()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        QQC2.ToolButton {
            icon.name: "media-skip-forward"
            text: Soma.favorites.length > 0 ? qsTr("Next favorite") : qsTr("Next station")
            display: QQC2.AbstractButton.IconOnly
            onClicked: Player.next()
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        Kirigami.Separator {
            Layout.fillHeight: true
            Layout.margins: Kirigami.Units.smallSpacing
        }

        QQC2.ComboBox {
            visible: bar.wide
            model: Player.qualities
            currentIndex: Player.quality
            onActivated: index => Player.quality = index
            QQC2.ToolTip.text: qsTr("Stream quality")
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        QQC2.ToolButton {
            icon.name: Player.muted || Player.volume === 0 ? "audio-volume-muted"
                     : Player.volume < 0.34 ? "audio-volume-low"
                     : Player.volume < 0.67 ? "audio-volume-medium" : "audio-volume-high"
            text: Player.muted ? qsTr("Unmute") : qsTr("Mute")
            display: QQC2.AbstractButton.IconOnly
            onClicked: Player.muted = !Player.muted
            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }
        QQC2.Slider {
            Layout.preferredWidth: Kirigami.Units.gridUnit * (bar.wide ? 7 : 4)
            from: 0
            to: 1
            value: Player.volume
            onMoved: {
                Player.muted = false
                Player.volume = value
            }
            WheelHandler {
                onWheel: event => Player.volume = Player.volume + (event.angleDelta.y > 0 ? 0.05 : -0.05)
            }
        }
    }
}
