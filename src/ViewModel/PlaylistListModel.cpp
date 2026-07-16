#include "ViewModel/PlaylistListModel.h"

#include <utility>

namespace AudioPlayer::ViewModel {

PlaylistListModel::PlaylistListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_playlists.size();
}

QVariant PlaylistListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_playlists.size()) {
        return {};
    }

    const Model::LibraryPlaylist &playlist = m_playlists.at(index.row());
    switch (role) {
    case IdRole:
        return playlist.id();
    case NameRole:
    case Qt::DisplayRole:
        return playlist.name();
    case SongCountRole:
        return playlist.size();
    case CreatedAtRole:
        return playlist.createdAt();
    case UpdatedAtRole:
        return playlist.updatedAt();
    default:
        return {};
    }
}

QHash<int, QByteArray> PlaylistListModel::roleNames() const
{
    return {
        {IdRole, "id"},
        {NameRole, "name"},
        {SongCountRole, "songCount"},
        {CreatedAtRole, "createdAt"},
        {UpdatedAtRole, "updatedAt"},
    };
}

const QVector<Model::LibraryPlaylist> &PlaylistListModel::playlists() const noexcept
{
    return m_playlists;
}

void PlaylistListModel::setPlaylists(QVector<Model::LibraryPlaylist> playlists)
{
    beginResetModel();
    m_playlists = std::move(playlists);
    endResetModel();
}

} // namespace AudioPlayer::ViewModel
