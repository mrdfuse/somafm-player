#include "player.h"
#include "somafm.h"

#include <QAudio>
#include <QAudioOutput>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSettings>

namespace {
struct Quality {
    const char *label;
    const char *format;
    const char *quality;
};
constexpr Quality kQualities[] = {
    {"MP3 · 128 kbps", "mp3", "highest"},
    {"AAC · 128 kbps", "aac", "highest"},
    {"AAC+ · 64 kbps", "aacp", "high"},
    {"AAC+ · 32 kbps", "aacp", "low"},
};
constexpr int kQualityCount = int(std::size(kQualities));
}

Player::Player(SomaFM *soma, QObject *parent)
    : QObject(parent)
    , m_soma(soma)
    , m_nam(new QNetworkAccessManager(this))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audio);

    QSettings settings;
    m_volume = qBound(0.0, settings.value(QStringLiteral("volume"), 0.8).toReal(), 1.0);
    m_quality = qBound(0, settings.value(QStringLiteral("quality"), 1).toInt(), kQualityCount - 1);
    m_channelId = settings.value(QStringLiteral("lastChannel")).toString();
    applyVolume();
    updateChannel();

    connect(m_soma, &SomaFM::channelsUpdated, this, [this] {
        updateChannel();
        if (m_title.isEmpty() && !m_channelId.isEmpty())
            setTrackFromString(m_channel.value(QStringLiteral("lastPlaying")).toString());
    });

    connect(m_soma, &SomaFM::songsReceived, this, [this](const QString &id, const QVariantList &songs) {
        if (id != m_channelId)
            return;
        m_history = songs;
        Q_EMIT historyChanged();
        if (!songs.isEmpty()) {
            const QVariantMap s = songs.first().toMap();
            setTrack(s.value(QStringLiteral("artist")).toString(), s.value(QStringLiteral("title")).toString(),
                     s.value(QStringLiteral("album")).toString());
        }
    });

    m_songTimer.setInterval(20 * 1000);
    connect(&m_songTimer, &QTimer::timeout, this, [this] { m_soma->fetchSongs(m_channelId); });

    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (!m_playing)
            return;
        switch (status) {
        case QMediaPlayer::BufferedMedia:
            m_reconnects = 0;
            setState(true, false);
            break;
        case QMediaPlayer::LoadingMedia:
        case QMediaPlayer::BufferingMedia:
        case QMediaPlayer::StalledMedia:
            setState(true, true);
            break;
        case QMediaPlayer::EndOfMedia: {
            // A live stream should never end; the server dropped us. Reconnect.
            setState(true, true);
            const quint64 gen = m_generation;
            QTimer::singleShot(qMin(2000 * (1 << qMin(m_reconnects, 4)), 30000), this, [this, gen] {
                if (gen == m_generation && m_playing)
                    startStream();
            });
            ++m_reconnects;
            break;
        }
        default:
            break;
        }
    });

    connect(m_player, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error, const QString &message) {
        if (!m_playing)
            return;
        // Try the next mirror from the playlist before giving up.
        if (++m_streamIndex < m_streams.size()) {
            startStream();
            return;
        }
        setError(message.isEmpty() ? tr("Playback failed") : message);
        m_player->stop();
        setState(false, false);
        m_songTimer.stop();
    });

    // ICY stream metadata gives us track changes instantly; the song API fills in the album.
    connect(m_player, &QMediaPlayer::metaDataChanged, this, [this] {
        const QString t = m_player->metaData().stringValue(QMediaMetaData::Title);
        if (t.isEmpty())
            return;
        setTrackFromString(t);
        const quint64 gen = m_generation;
        QTimer::singleShot(4000, this, [this, gen] {
            if (gen == m_generation)
                m_soma->fetchSongs(m_channelId);
        });
    });
}

Player::~Player()
{
    QSettings settings;
    settings.setValue(QStringLiteral("volume"), m_volume);
    settings.setValue(QStringLiteral("quality"), m_quality);
    settings.setValue(QStringLiteral("lastChannel"), m_channelId);
}

QStringList Player::qualities() const
{
    QStringList list;
    for (const Quality &q : kQualities)
        list << QString::fromUtf8(q.label);
    return list;
}

void Player::updateChannel()
{
    const QVariantMap c = m_soma->channel(m_channelId);
    if (c == m_channel)
        return;
    m_channel = c;
    Q_EMIT channelChanged();
}

QUrl Player::playlistUrl() const
{
    const Channel *c = m_soma->find(m_channelId);
    if (!c || c->playlists.isEmpty())
        return {};
    const Quality &want = kQualities[m_quality];
    for (const QVariant &v : c->playlists) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("format")).toString() == QLatin1String(want.format)
            && p.value(QStringLiteral("quality")).toString() == QLatin1String(want.quality))
            return p.value(QStringLiteral("url")).toUrl();
    }
    // Fall back to the same format at any quality, then to anything.
    for (const QVariant &v : c->playlists) {
        const QVariantMap p = v.toMap();
        if (p.value(QStringLiteral("format")).toString() == QLatin1String(want.format))
            return p.value(QStringLiteral("url")).toUrl();
    }
    return c->playlists.first().toMap().value(QStringLiteral("url")).toUrl();
}

void Player::play(const QString &id)
{
    if (!m_soma->find(id))
        return;

    const bool changed = id != m_channelId;
    ++m_generation;
    m_player->stop();
    m_streams.clear();
    m_reconnects = 0;
    setError({});

    if (changed) {
        m_channelId = id;
        QSettings().setValue(QStringLiteral("lastChannel"), id);
        updateChannel();
        m_history.clear();
        Q_EMIT historyChanged();
        m_title.clear();
        setTrackFromString(m_channel.value(QStringLiteral("lastPlaying")).toString());
    }
    setState(true, true);

    const QUrl pls = playlistUrl();
    QNetworkRequest req(pls);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("somafm-player/1.0 (KDE)"));
    req.setTransferTimeout(15000);
    QNetworkReply *reply = m_nam->get(req);
    const quint64 gen = m_generation;
    connect(reply, &QNetworkReply::finished, this, [this, reply, gen] {
        reply->deleteLater();
        if (gen != m_generation)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            setError(tr("Could not load the stream: %1").arg(reply->errorString()));
            setState(false, false);
            return;
        }
        static const QRegularExpression fileLine(QStringLiteral("^File\\d+=(.+)$"),
                                                 QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption);
        auto it = fileLine.globalMatch(QString::fromUtf8(reply->readAll()));
        while (it.hasNext())
            m_streams.append(QUrl(it.next().captured(1).trimmed()));
        if (m_streams.isEmpty()) {
            setError(tr("This station has no streams available"));
            setState(false, false);
            return;
        }
        m_streamIndex = 0;
        startStream();
    });

    m_soma->fetchSongs(m_channelId);
    m_songTimer.start();
}

void Player::startStream()
{
    m_player->stop();
    m_player->setSource(m_streams.value(m_streamIndex));
    m_player->play();
}

void Player::stop()
{
    ++m_generation;
    m_player->stop();
    m_player->setSource({});
    m_songTimer.stop();
    setState(false, false);
}

void Player::toggle()
{
    if (m_playing)
        stop();
    else if (!m_channelId.isEmpty())
        play(m_channelId);
}

void Player::next()
{
    const QStringList ids = m_soma->stationCycle();
    if (ids.isEmpty())
        return;
    const int i = ids.indexOf(m_channelId);
    play(ids.at((i + 1) % ids.size()));
}

void Player::previous()
{
    const QStringList ids = m_soma->stationCycle();
    if (ids.isEmpty())
        return;
    const int i = ids.indexOf(m_channelId);
    play(ids.at(i <= 0 ? ids.size() - 1 : i - 1));
}

void Player::setState(bool playing, bool buffering)
{
    if (playing == m_playing && buffering == m_buffering)
        return;
    m_playing = playing;
    m_buffering = buffering;
    Q_EMIT stateChanged();
}

void Player::setError(const QString &error)
{
    if (error == m_error)
        return;
    m_error = error;
    Q_EMIT errorChanged();
}

void Player::setTrack(const QString &artist, const QString &title, const QString &album)
{
    if (artist == m_artist && title == m_title && album == m_album)
        return;
    m_artist = artist;
    m_title = title;
    m_album = album;
    Q_EMIT trackChanged();
}

void Player::setTrackFromString(const QString &text)
{
    const int sep = text.indexOf(QLatin1String(" - "));
    if (sep < 0) {
        setTrack({}, text.trimmed(), {});
        return;
    }
    const QString artist = text.left(sep).trimmed();
    const QString title = text.mid(sep + 3).trimmed();
    // Keep the album when this is the same track we already know about.
    setTrack(artist, title, artist == m_artist && title == m_title ? m_album : QString());
}

void Player::setVolume(qreal volume)
{
    volume = qBound(0.0, volume, 1.0);
    if (qFuzzyCompare(volume, m_volume))
        return;
    m_volume = volume;
    QSettings().setValue(QStringLiteral("volume"), m_volume);
    applyVolume();
    Q_EMIT volumeChanged();
}

void Player::setMuted(bool muted)
{
    if (muted == m_muted)
        return;
    m_muted = muted;
    applyVolume();
    Q_EMIT volumeChanged();
}

void Player::applyVolume()
{
    // The slider is perceptual; the audio output expects linear gain.
    m_audio->setVolume(QAudio::convertVolume(m_volume, QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale));
    m_audio->setMuted(m_muted);
}

void Player::setQuality(int quality)
{
    quality = qBound(0, quality, kQualityCount - 1);
    if (quality == m_quality)
        return;
    m_quality = quality;
    QSettings().setValue(QStringLiteral("quality"), m_quality);
    Q_EMIT qualityChanged();
    if (m_playing)
        play(m_channelId);
}
