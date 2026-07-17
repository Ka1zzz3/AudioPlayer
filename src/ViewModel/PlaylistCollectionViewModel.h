#pragma once

#include "ViewModel/PlaylistCollectionViewModelProtocol.h"
#include "ViewModel/PlaylistListModel.h"

#include "Model/LibraryDocument.h"
#include "Model/Service/PlaylistCollectionUseCase.h"

#include <memory>

namespace AudioPlayer::ViewModel {

namespace ModelService = AudioPlayer::Model::Service;

class PlaylistCollectionViewModel : public PlaylistCollectionViewModelProtocol
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *playlists READ playlists CONSTANT)
    Q_PROPERTY(QString storagePath READ storagePath WRITE setStoragePath NOTIFY storagePathChanged)
    Q_PROPERTY(QString newPlaylistName READ newPlaylistName WRITE setNewPlaylistName NOTIFY newPlaylistNameChanged)
    Q_PROPERTY(QString visiblePlaylistId READ visiblePlaylistId NOTIFY visiblePlaylistChanged)
    Q_PROPERTY(QString visiblePlaylistName READ visiblePlaylistName NOTIFY visiblePlaylistChanged)
    Q_PROPERTY(int playlistCount READ playlistCount NOTIFY playlistCountChanged)
    Q_PROPERTY(QString selectedPlaylistId READ selectedPlaylistId WRITE setSelectedPlaylistId NOTIFY selectedPlaylistChanged)
    Q_PROPERTY(int selectedSongIndex READ selectedSongIndex WRITE setSelectedSongIndex NOTIFY selectedSongIndexChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *loadCommand READ loadCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *saveCommand READ saveCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *createPlaylistCommand READ createPlaylistCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *deletePlaylistCommand READ deletePlaylistCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *switchPlaylistCommand READ switchPlaylistCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *addSongsCommand READ addSongsCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *playSelectedSongCommand READ playSelectedSongCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *playVisiblePlaylistCommand READ playVisiblePlaylistCommand CONSTANT)

public:
    explicit PlaylistCollectionViewModel(std::shared_ptr<const ModelService::PlaylistCollectionUseCase> useCase,
                                         QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *playlists() noexcept override;
    [[nodiscard]] int playlistIdRole() const noexcept override;
    [[nodiscard]] const QString &storagePath() const noexcept override;
    void setStoragePath(QString storagePath) override;
    [[nodiscard]] const QString &newPlaylistName() const noexcept override;
    void setNewPlaylistName(QString name) override;
    [[nodiscard]] const QString &visiblePlaylistId() const noexcept override;
    [[nodiscard]] const QString &visiblePlaylistName() const noexcept override;
    [[nodiscard]] int playlistCount() const noexcept override;
    [[nodiscard]] const QString &selectedPlaylistId() const noexcept override;
    void setSelectedPlaylistId(QString playlistId) override;
    [[nodiscard]] int selectedSongIndex() const noexcept override;
    void setSelectedSongIndex(int index) override;
    [[nodiscard]] const QString &lastError() const noexcept override;
    [[nodiscard]] const QString &statusMessage() const noexcept override;

    [[nodiscard]] Common::ViewCommand *loadCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *saveCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *createPlaylistCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *deletePlaylistCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *switchPlaylistCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *addSongsCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *playSelectedSongCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *playVisiblePlaylistCommand() noexcept override;

    void setCurrentVisibleSongs(QVector<Model::AudioFile> songs);
    [[nodiscard]] QVector<Model::AudioFile> currentVisibleSongsSnapshot() const;

signals:
    void currentVisibleSongsChanged(const QVector<AudioPlayer::Model::AudioFile> &songs);
    void playRequested(const QString &playlistId,
                       const QVector<AudioPlayer::Model::AudioFile> &songsSnapshot,
                       int startIndex);

private:
    bool executeLoad();
    bool executeSave();
    bool executeCreatePlaylist();
    bool executeDeletePlaylist();
    bool executeSwitchPlaylist();
    bool executeAddSongs();
    bool executePlaySelectedSong();
    bool executePlayVisiblePlaylist();
    void applyResult(ModelService::PlaylistCollectionResult result);
    void setDocument(Model::LibraryDocument document);
    void emitVisibleSongsChanged();
    void setLastError(QString error);
    void setStatusMessage(QString status);
    QString nextPlaylistId() const;

    PlaylistListModel m_playlists;
    std::shared_ptr<const ModelService::PlaylistCollectionUseCase> m_useCase;
    Common::ViewCommand m_loadCommand;
    Common::ViewCommand m_saveCommand;
    Common::ViewCommand m_createPlaylistCommand;
    Common::ViewCommand m_deletePlaylistCommand;
    Common::ViewCommand m_switchPlaylistCommand;
    Common::ViewCommand m_addSongsCommand;
    Common::ViewCommand m_playSelectedSongCommand;
    Common::ViewCommand m_playVisiblePlaylistCommand;
    Model::LibraryDocument m_document;
    QVector<Model::AudioFile> m_currentVisibleSongs;
    QString m_storagePath;
    QString m_newPlaylistName;
    QString m_selectedPlaylistId;
    int m_selectedSongIndex = 0;
    QString m_lastError;
    QString m_statusMessage;
};

} // namespace AudioPlayer::ViewModel
