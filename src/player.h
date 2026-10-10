#pragma once

#include <QObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVariant>

class QAudioOutput;
class QMediaPlayer;
class QNetworkAccessManager;
class SomaFM;

class Player : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString channelId READ channelId NOTIFY channelChanged)
    Q_PROPERTY(QVariantMap channel READ channel NOTIFY channelChanged)
    Q_PROPERTY(bool playing READ playing NOTIFY stateChanged)
    Q_PROPERTY(bool buffering READ buffering NOTIFY stateChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY volumeChanged)
    Q_PROPERTY(int quality READ quality WRITE setQuality NOTIFY qualityChanged)
    Q_PROPERTY(QStringList qualities READ qualities CONSTANT)
    Q_PROPERTY(QString artist READ artist NOTIFY trackChanged)
    Q_PROPERTY(QString title READ title NOTIFY trackChanged)
    Q_PROPERTY(QString album READ album NOTIFY trackChanged)
    Q_PROPERTY(QVariantList history READ history NOTIFY historyChanged)

public:
    explicit Player(SomaFM *soma, QObject *parent = nullptr);
    ~Player() override;

    QString channelId() const { return m_channelId; }
    QVariantMap channel() const { return m_channel; }
    bool playing() const { return m_playing; }
    bool buffering() const { return m_buffering; }
    QString error() const { return m_error; }
    qreal volume() const { return m_volume; }
    void setVolume(qreal volume);
    bool muted() const { return m_muted; }
    void setMuted(bool muted);
    int quality() const { return m_quality; }
    void setQuality(int quality);
    QStringList qualities() const;
    QString artist() const { return m_artist; }
    QString title() const { return m_title; }
    QString album() const { return m_album; }
    QVariantList history() const { return m_history; }

    Q_INVOKABLE void play(const QString &id);
    Q_INVOKABLE void stop();
    Q_INVOKABLE void toggle();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();

Q_SIGNALS:
    void channelChanged();
    void stateChanged();
    void errorChanged();
    void volumeChanged();
    void qualityChanged();
    void trackChanged();
    void historyChanged();
    void raiseRequested();

private:
    void startStream();
    void setState(bool playing, bool buffering);
    void setError(const QString &error);
    void setTrack(const QString &artist, const QString &title, const QString &album);
    void setTrackFromString(const QString &text);
    void applyVolume();
    void updateChannel();
    QUrl playlistUrl() const;

    SomaFM *m_soma;
    QNetworkAccessManager *m_nam;
    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    QTimer m_songTimer;

    QString m_channelId;
    QVariantMap m_channel;
    QList<QUrl> m_streams;
    int m_streamIndex = 0;
    int m_reconnects = 0;
    quint64 m_generation = 0;
    bool m_playing = false;
    bool m_buffering = false;
    QString m_error;
    qreal m_volume = 0.8;
    bool m_muted = false;
    int m_quality = 1;
    QString m_artist, m_title, m_album;
    QVariantList m_history;
};
