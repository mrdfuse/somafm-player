#include "somafm.h"

#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSettings>
#include <QStandardPaths>

static QNetworkRequest apiRequest(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("somafm-player/1.0 (KDE)"));
    req.setTransferTimeout(15000);
    return req;
}

// ChannelModel

int ChannelModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_channels.size();
}

QVariant ChannelModel::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid))
        return {};
    const Channel &c = m_channels.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole: return c.title;
    case ChannelIdRole: return c.id;
    case DescriptionRole: return c.description;
    case DjRole: return c.dj;
    case DjMailRole: return c.djmail;
    case GenreRole: return c.genres.join(QStringLiteral(", "));
    case GenresRole: return c.genres;
    case ImageRole: return c.image;
    case LargeImageRole: return c.largeImage;
    case XlImageRole: return c.xlImage;
    case ListenersRole: return c.listeners;
    case LastPlayingRole: return c.lastPlaying;
    case FavoriteRole: return m_favorites.contains(c.id);
    }
    return {};
}

QHash<int, QByteArray> ChannelModel::roleNames() const
{
    return {
        {ChannelIdRole, "channelId"},
        {TitleRole, "title"},
        {DescriptionRole, "description"},
        {DjRole, "dj"},
        {DjMailRole, "djmail"},
        {GenreRole, "genre"},
        {GenresRole, "genres"},
        {ImageRole, "image"},
        {LargeImageRole, "largeImage"},
        {XlImageRole, "xlImage"},
        {ListenersRole, "listeners"},
        {LastPlayingRole, "lastPlaying"},
        {FavoriteRole, "favorite"},
    };
}

void ChannelModel::setChannels(const QList<Channel> &channels)
{
    bool sameIds = channels.size() == m_channels.size();
    for (int i = 0; sameIds && i < channels.size(); ++i)
        sameIds = channels[i].id == m_channels[i].id;

    if (sameIds) {
        // Update in place so views keep their scroll position and selection.
        m_channels = channels;
        if (!m_channels.isEmpty())
            Q_EMIT dataChanged(index(0), index(m_channels.size() - 1));
    } else {
        beginResetModel();
        m_channels = channels;
        endResetModel();
    }
}

void ChannelModel::setFavorites(const QStringList &favorites)
{
    m_favorites = favorites;
    if (!m_channels.isEmpty())
        Q_EMIT dataChanged(index(0), index(m_channels.size() - 1), {FavoriteRole});
}

const Channel *ChannelModel::find(const QString &id) const
{
    for (const Channel &c : m_channels)
        if (c.id == id)
            return &c;
    return nullptr;
}

// ChannelFilter

ChannelFilter::ChannelFilter(ChannelModel *source, QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSourceModel(source);
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    sort(0);
    connect(this, &QAbstractItemModel::rowsInserted, this, &ChannelFilter::countChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ChannelFilter::countChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &ChannelFilter::countChanged);
    connect(this, &QAbstractItemModel::layoutChanged, this, &ChannelFilter::countChanged);
}

void ChannelFilter::refilter()
{
    invalidate();
    Q_EMIT filterChanged();
    Q_EMIT countChanged();
}

void ChannelFilter::setFilterText(const QString &text)
{
    if (text == m_text)
        return;
    m_text = text;
    refilter();
}

void ChannelFilter::setFavoritesOnly(bool on)
{
    if (on == m_favoritesOnly)
        return;
    m_favoritesOnly = on;
    refilter();
}

void ChannelFilter::setGenre(const QString &genre)
{
    if (genre == m_genre)
        return;
    m_genre = genre;
    refilter();
}

void ChannelFilter::setSortMode(int mode)
{
    if (mode == m_sortMode)
        return;
    m_sortMode = mode;
    refilter();
}

bool ChannelFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
    if (m_favoritesOnly && !idx.data(ChannelModel::FavoriteRole).toBool())
        return false;
    if (!m_genre.isEmpty() && !idx.data(ChannelModel::GenresRole).toStringList().contains(m_genre))
        return false;
    const QString text = m_text.trimmed();
    if (text.isEmpty())
        return true;
    for (int role : {ChannelModel::TitleRole, ChannelModel::DescriptionRole, ChannelModel::GenreRole,
                     ChannelModel::DjRole, ChannelModel::LastPlayingRole}) {
        if (idx.data(role).toString().contains(text, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool ChannelFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto title = [](const QModelIndex &i) { return i.data(ChannelModel::TitleRole).toString(); };
    if (m_sortMode == ByListeners) {
        const int l = left.data(ChannelModel::ListenersRole).toInt();
        const int r = right.data(ChannelModel::ListenersRole).toInt();
        if (l != r)
            return l > r;
    } else if (m_sortMode == ByGenre) {
        const int c = QString::localeAwareCompare(left.data(ChannelModel::GenreRole).toString(),
                                                  right.data(ChannelModel::GenreRole).toString());
        if (c != 0)
            return c < 0;
    }
    return QString::localeAwareCompare(title(left), title(right)) < 0;
}

// SomaFM

SomaFM::SomaFM(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_model(new ChannelModel(this))
    , m_filter(new ChannelFilter(m_model, this))
{
    m_favorites = QSettings().value(QStringLiteral("favorites")).toStringList();
    m_model->setFavorites(m_favorites);

    // Show the last known station list immediately, then refresh from the network.
    QFile cache(cacheFile());
    if (cache.open(QIODevice::ReadOnly))
        parseChannels(cache.readAll());

    m_refreshTimer.setInterval(60 * 1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &SomaFM::refresh);
    m_refreshTimer.start();
    refresh();
}

QString SomaFM::cacheFile() const
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + QStringLiteral("/channels.json");
}

void SomaFM::setError(const QString &error)
{
    if (error == m_error)
        return;
    m_error = error;
    Q_EMIT errorChanged();
}

void SomaFM::refresh()
{
    if (m_loading)
        return;
    m_loading = true;
    Q_EMIT loadingChanged();

    QNetworkReply *reply = m_nam->get(apiRequest(QUrl(QStringLiteral("https://somafm.com/channels.json"))));
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        m_loading = false;
        Q_EMIT loadingChanged();
        if (reply->error() != QNetworkReply::NoError) {
            setError(tr("Could not load stations: %1").arg(reply->errorString()));
            return;
        }
        const QByteArray json = reply->readAll();
        if (!parseChannels(json)) {
            setError(tr("Received an invalid station list"));
            return;
        }
        setError({});
        QDir().mkpath(QFileInfo(cacheFile()).path());
        QFile cache(cacheFile());
        if (cache.open(QIODevice::WriteOnly | QIODevice::Truncate))
            cache.write(json);
    });
}

bool SomaFM::parseChannels(const QByteArray &json)
{
    const QJsonArray array = QJsonDocument::fromJson(json).object().value(QStringLiteral("channels")).toArray();
    if (array.isEmpty())
        return false;

    QList<Channel> channels;
    QStringList genres;
    for (const QJsonValue &v : array) {
        const QJsonObject o = v.toObject();
        Channel c;
        c.id = o.value(QStringLiteral("id")).toString();
        if (c.id.isEmpty())
            continue;
        c.title = o.value(QStringLiteral("title")).toString();
        c.description = o.value(QStringLiteral("description")).toString();
        c.dj = o.value(QStringLiteral("dj")).toString();
        c.djmail = o.value(QStringLiteral("djmail")).toString();
        c.genre = o.value(QStringLiteral("genre")).toString();
        c.genres = c.genre.split(QLatin1Char('|'), Qt::SkipEmptyParts);
        c.image = o.value(QStringLiteral("image")).toString();
        c.largeImage = o.value(QStringLiteral("largeimage")).toString();
        c.xlImage = o.value(QStringLiteral("xlimage")).toString();
        c.listeners = o.value(QStringLiteral("listeners")).toVariant().toInt();
        c.lastPlaying = o.value(QStringLiteral("lastPlaying")).toString();
        for (const QJsonValue &p : o.value(QStringLiteral("playlists")).toArray())
            c.playlists.append(p.toObject().toVariantMap());
        for (const QString &g : std::as_const(c.genres))
            if (!genres.contains(g))
                genres.append(g);
        channels.append(c);
    }
    genres.sort(Qt::CaseInsensitive);

    m_model->setChannels(channels);
    m_genres = genres;
    Q_EMIT channelsUpdated();
    return true;
}

void SomaFM::fetchSongs(const QString &id)
{
    if (id.isEmpty())
        return;
    const QUrl url(QStringLiteral("https://somafm.com/songs/%1.json").arg(id));
    QNetworkReply *reply = m_nam->get(apiRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, id] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError)
            return;
        QVariantList songs;
        const QJsonArray array = QJsonDocument::fromJson(reply->readAll()).object().value(QStringLiteral("songs")).toArray();
        for (const QJsonValue &v : array) {
            const QJsonObject o = v.toObject();
            songs.append(QVariantMap{
                {QStringLiteral("title"), o.value(QStringLiteral("title")).toString()},
                {QStringLiteral("artist"), o.value(QStringLiteral("artist")).toString()},
                {QStringLiteral("album"), o.value(QStringLiteral("album")).toString()},
                {QStringLiteral("albumArt"), o.value(QStringLiteral("albumArt")).toString()},
                {QStringLiteral("date"), o.value(QStringLiteral("date")).toVariant().toLongLong()},
            });
        }
        Q_EMIT songsReceived(id, songs);
    });
}

QVariantMap SomaFM::channel(const QString &id) const
{
    const Channel *c = m_model->find(id);
    if (!c)
        return {};
    return {
        {QStringLiteral("channelId"), c->id},
        {QStringLiteral("title"), c->title},
        {QStringLiteral("description"), c->description},
        {QStringLiteral("dj"), c->dj},
        {QStringLiteral("djmail"), c->djmail},
        {QStringLiteral("genre"), c->genres.join(QStringLiteral(", "))},
        {QStringLiteral("image"), c->image},
        {QStringLiteral("largeImage"), c->largeImage},
        {QStringLiteral("xlImage"), c->xlImage},
        {QStringLiteral("listeners"), c->listeners},
        {QStringLiteral("lastPlaying"), c->lastPlaying},
    };
}

void SomaFM::toggleFavorite(const QString &id)
{
    if (id.isEmpty())
        return;
    if (!m_favorites.removeOne(id))
        m_favorites.append(id);
    QSettings().setValue(QStringLiteral("favorites"), m_favorites);
    m_model->setFavorites(m_favorites);
    Q_EMIT favoritesChanged();
}

void SomaFM::copyToClipboard(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

QStringList SomaFM::stationCycle() const
{
    QStringList ids;
    // Follow the order the user currently sees in the list.
    for (int row = 0; row < m_filter->rowCount(); ++row) {
        const QString id = m_filter->index(row, 0).data(ChannelModel::ChannelIdRole).toString();
        if (m_favorites.isEmpty() || m_favorites.contains(id))
            ids.append(id);
    }
    if (ids.isEmpty()) {
        for (const Channel &c : m_model->channels())
            if (m_favorites.isEmpty() || m_favorites.contains(c.id))
                ids.append(c.id);
    }
    return ids;
}
