#pragma once

#include "Common/ViewCommand.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>

namespace AudioPlayer::ViewModel {

class PlaylistCollectionViewModelProtocol : public QObject
{
    Q_OBJECT

public:
    explicit PlaylistCollectionViewModelProtocol(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] virtual QAbstractItemModel *playlists() noexcept = 0;
    [[nodiscard]] virtual int playlistIdRole() const noexcept = 0;
    [[nodiscard]] virtual const QString &storagePath() const noexcept = 0;
    virtual void setStoragePath(QString storagePath) = 0;
    [[nodiscard]] virtual const QString &newPlaylistName() const noexcept = 0;
    virtual void setNewPlaylistName(QString name) = 0;
    [[nodiscard]] virtual const QString &visiblePlaylistId() const noexcept = 0;
    [[nodiscard]] virtual const QString &visiblePlaylistName() const noexcept = 0;
    [[nodiscard]] virtual int playlistCount() const noexcept = 0;
    [[nodiscard]] virtual const QString &selectedPlaylistId() const noexcept = 0;
    virtual void setSelectedPlaylistId(QString playlistId) = 0;
    [[nodiscard]] virtual int selectedSongIndex() const noexcept = 0;
    virtual void setSelectedSongIndex(int index) = 0;
    [[nodiscard]] virtual const QString &lastError() const noexcept = 0;
    [[nodiscard]] virtual const QString &statusMessage() const noexcept = 0;

    [[nodiscard]] virtual Common::ViewCommand *loadCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *saveCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *createPlaylistCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *deletePlaylistCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *switchPlaylistCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *addSongsCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *playSelectedSongCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *playVisiblePlaylistCommand() noexcept = 0;

signals:
    void storagePathChanged();
    void newPlaylistNameChanged();
    void visiblePlaylistChanged();
    void playlistCountChanged();
    void selectedPlaylistChanged();
    void selectedSongIndexChanged();
    void lastErrorChanged();
    void statusMessageChanged();
};

} // namespace AudioPlayer::ViewModel
