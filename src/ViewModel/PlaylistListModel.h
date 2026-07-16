#pragma once

#include "Model/LibraryPlaylist.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

namespace AudioPlayer::ViewModel {

class PlaylistListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum PlaylistRole {
        IdRole = Qt::UserRole + 1,
        NameRole,
        SongCountRole,
        CreatedAtRole,
        UpdatedAtRole,
    };
    Q_ENUM(PlaylistRole)

    explicit PlaylistListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const QVector<Model::LibraryPlaylist> &playlists() const noexcept;
    void setPlaylists(QVector<Model::LibraryPlaylist> playlists);

private:
    QVector<Model::LibraryPlaylist> m_playlists;
};

} // namespace AudioPlayer::ViewModel
