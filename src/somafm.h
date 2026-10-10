#pragma once

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QStringList>
#include <QTimer>
#include <QVariant>

class QNetworkAccessManager;

struct Channel {
    QString id, title, description, dj, djmail, genre;
    QStringList genres;
    QString image, largeImage, xlImage;
    int listeners = 0;
    QString lastPlaying;
    QVariantList playlists;
};

class ChannelModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        ChannelIdRole = Qt::UserRole + 1,
        TitleRole,
        DescriptionRole,
        DjRole,
        DjMailRole,
        GenreRole,
        GenresRole,
        ImageRole,
        LargeImageRole,
        XlImageRole,
        ListenersRole,
        LastPlayingRole,
        FavoriteRole,
    };

    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setChannels(const QList<Channel> &channels);
    void setFavorites(const QStringList &favorites);
    const QList<Channel> &channels() const { return m_channels; }
    const Channel *find(const QString &id) const;

private:
    QList<Channel> m_channels;
    QStringList m_favorites;
};

class ChannelFilter : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterChanged)
    Q_PROPERTY(bool favoritesOnly READ favoritesOnly WRITE setFavoritesOnly NOTIFY filterChanged)
    Q_PROPERTY(QString genre READ genre WRITE setGenre NOTIFY filterChanged)
    Q_PROPERTY(int sortMode READ sortMode WRITE setSortMode NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum SortMode { ByName, ByListeners, ByGenre };
    Q_ENUM(SortMode)

    explicit ChannelFilter(ChannelModel *source, QObject *parent = nullptr);

    QString filterText() const { return m_text; }
    void setFilterText(const QString &text);
    bool favoritesOnly() const { return m_favoritesOnly; }
    void setFavoritesOnly(bool on);
    QString genre() const { return m_genre; }
    void setGenre(const QString &genre);
    int sortMode() const { return m_sortMode; }
    void setSortMode(int mode);
    int count() const { return rowCount(); }

Q_SIGNALS:
    void filterChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    void refilter();

    QString m_text;
    bool m_favoritesOnly = false;
    QString m_genre;
    int m_sortMode = ByName;
};

class SomaFM : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ChannelFilter *channels READ channels CONSTANT)
    Q_PROPERTY(QStringList genres READ genres NOTIFY channelsUpdated)
    Q_PROPERTY(QStringList favorites READ favorites NOTIFY favoritesChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit SomaFM(QObject *parent = nullptr);

    ChannelFilter *channels() const { return m_filter; }
    QStringList genres() const { return m_genres; }
    QStringList favorites() const { return m_favorites; }
    bool loading() const { return m_loading; }
    QString error() const { return m_error; }
    const Channel *find(const QString &id) const { return m_model->find(id); }

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void fetchSongs(const QString &id);
    Q_INVOKABLE QVariantMap channel(const QString &id) const;
    Q_INVOKABLE bool isFavorite(const QString &id) const { return m_favorites.contains(id); }
    Q_INVOKABLE void toggleFavorite(const QString &id);
    Q_INVOKABLE void copyToClipboard(const QString &text);

    // Stations that next/previous cycle through: favorites if any, else all.
    QStringList stationCycle() const;

Q_SIGNALS:
    void channelsUpdated();
    void favoritesChanged();
    void loadingChanged();
    void errorChanged();
    void songsReceived(const QString &id, const QVariantList &songs);

private:
    bool parseChannels(const QByteArray &json);
    void setError(const QString &error);
    QString cacheFile() const;

    QNetworkAccessManager *m_nam;
    ChannelModel *m_model;
    ChannelFilter *m_filter;
    QStringList m_genres;
    QStringList m_favorites;
    bool m_loading = false;
    QString m_error;
    QTimer m_refreshTimer;
};
