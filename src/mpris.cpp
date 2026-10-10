#include "mpris.h"
#include "player.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QMetaProperty>

static const QString kPath = QStringLiteral("/org/mpris/MediaPlayer2");

MprisRoot::MprisRoot(QObject *parent, Player *player)
    : QDBusAbstractAdaptor(parent)
    , m_player(player)
{
}

void MprisRoot::Raise()
{
    Q_EMIT m_player->raiseRequested();
}

void MprisRoot::Quit()
{
    QCoreApplication::quit();
}

MprisPlayer::MprisPlayer(QObject *parent, Player *player)
    : QDBusAbstractAdaptor(parent)
    , m_player(player)
{
    connect(player, &Player::stateChanged, this, [this] { notify({QStringLiteral("PlaybackStatus")}); });
    connect(player, &Player::trackChanged, this, [this] { notify({QStringLiteral("Metadata")}); });
    connect(player, &Player::channelChanged, this, [this] {
        notify({QStringLiteral("Metadata"), QStringLiteral("CanPlay"), QStringLiteral("CanPause")});
    });
    connect(player, &Player::volumeChanged, this, [this] { notify({QStringLiteral("Volume")}); });
}

QString MprisPlayer::playbackStatus() const
{
    return m_player->playing() ? QStringLiteral("Playing") : QStringLiteral("Stopped");
}

QVariantMap MprisPlayer::metadata() const
{
    if (m_player->channelId().isEmpty())
        return {};
    const QVariantMap ch = m_player->channel();
    const QString station = ch.value(QStringLiteral("title")).toString();
    QVariantMap md;
    md[QStringLiteral("mpris:trackid")] = QVariant::fromValue(
        QDBusObjectPath(QStringLiteral("/org/somafm/player/station/") + m_player->channelId()));
    md[QStringLiteral("xesam:title")] = m_player->title().isEmpty() ? station : m_player->title();
    if (!m_player->artist().isEmpty())
        md[QStringLiteral("xesam:artist")] = QStringList{m_player->artist()};
    md[QStringLiteral("xesam:album")] = m_player->album().isEmpty() ? station : m_player->album();
    const QString art = ch.value(QStringLiteral("xlImage")).toString();
    if (!art.isEmpty())
        md[QStringLiteral("mpris:artUrl")] = art;
    md[QStringLiteral("xesam:url")] = QStringLiteral("https://somafm.com/%1/").arg(m_player->channelId());
    return md;
}

double MprisPlayer::volume() const
{
    return m_player->muted() ? 0.0 : m_player->volume();
}

void MprisPlayer::setVolume(double volume)
{
    m_player->setMuted(false);
    m_player->setVolume(volume);
}

bool MprisPlayer::canPlay() const
{
    return !m_player->channelId().isEmpty();
}

void MprisPlayer::Next() { m_player->next(); }
void MprisPlayer::Previous() { m_player->previous(); }
void MprisPlayer::Pause() { m_player->stop(); }
void MprisPlayer::PlayPause() { m_player->toggle(); }
void MprisPlayer::Stop() { m_player->stop(); }

void MprisPlayer::Play()
{
    if (!m_player->playing())
        m_player->play(m_player->channelId());
}

void MprisPlayer::notify(const QStringList &properties)
{
    QVariantMap changed;
    for (const QString &name : properties) {
        const int idx = metaObject()->indexOfProperty(name.toLatin1().constData());
        if (idx >= 0)
            changed[name] = metaObject()->property(idx).read(this);
    }
    QDBusMessage msg = QDBusMessage::createSignal(kPath, QStringLiteral("org.freedesktop.DBus.Properties"),
                                                  QStringLiteral("PropertiesChanged"));
    msg << QStringLiteral("org.mpris.MediaPlayer2.Player") << changed << QStringList();
    QDBusConnection::sessionBus().send(msg);
}

bool registerMpris(Player *player)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected())
        return true; // No session bus: run without media-key integration.
    if (!bus.registerService(QStringLiteral("org.mpris.MediaPlayer2.somafm")))
        return false;
    auto *object = new QObject(player);
    new MprisRoot(object, player);
    new MprisPlayer(object, player);
    bus.registerObject(kPath, object);
    return true;
}
