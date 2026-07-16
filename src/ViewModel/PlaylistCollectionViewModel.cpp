#include "ViewModel/PlaylistCollectionViewModel.h"

#include "Model/JsonLibraryDocumentRepository.h"

#include <memory>
#include <utility>

#include <QDateTime>

namespace AudioPlayer::ViewModel {

PlaylistCollectionViewModel::PlaylistCollectionViewModel(QObject *parent)
    : PlaylistCollectionViewModel(std::make_shared<ModelService::PlaylistCollectionUseCase>(), parent)
{
}

PlaylistCollectionViewModel::PlaylistCollectionViewModel(
    std::shared_ptr<const ModelService::PlaylistCollectionUseCase> useCase,
    QObject *parent)
    : PlaylistCollectionViewModelProtocol(parent)
    , m_playlists(this)
    , m_useCase(std::move(useCase))
    , m_loadCommand(QStringLiteral("loadPlaylists"), [this]() { return executeLoad(); }, this)
    , m_saveCommand(QStringLiteral("savePlaylists"), [this]() { return executeSave(); }, this)
    , m_createPlaylistCommand(QStringLiteral("createPlaylist"), [this]() { return executeCreatePlaylist(); }, this)
    , m_deletePlaylistCommand(QStringLiteral("deletePlaylist"), [this]() { return executeDeletePlaylist(); }, this)
    , m_switchPlaylistCommand(QStringLiteral("switchPlaylist"), [this]() { return executeSwitchPlaylist(); }, this)
    , m_addSongsCommand(QStringLiteral("addSongs"), [this]() { return executeAddSongs(); }, this)
    , m_playSelectedSongCommand(QStringLiteral("playSelectedSong"),
                                [this]() { return executePlaySelectedSong(); },
                                this)
    , m_playVisiblePlaylistCommand(QStringLiteral("playVisiblePlaylist"),
                                   [this]() { return executePlayVisiblePlaylist(); },
                                   this)
    , m_storagePath(Model::JsonLibraryDocumentRepository::defaultStoragePath())
{
    if (!m_useCase) {
        m_useCase = std::make_shared<ModelService::PlaylistCollectionUseCase>();
    }
    ModelService::PlaylistCollectionResult result = m_useCase->createEmptyDocument(QStringLiteral("default"),
                                                                                   QDateTime::currentDateTimeUtc());
    setDocument(std::move(result.document));
}

QAbstractItemModel *PlaylistCollectionViewModel::playlists() noexcept
{
    return &m_playlists;
}

const QString &PlaylistCollectionViewModel::storagePath() const noexcept
{
    return m_storagePath;
}

void PlaylistCollectionViewModel::setStoragePath(QString storagePath)
{
    if (m_storagePath == storagePath) {
        return;
    }

    m_storagePath = std::move(storagePath);
    emit storagePathChanged();
}

const QString &PlaylistCollectionViewModel::newPlaylistName() const noexcept
{
    return m_newPlaylistName;
}

void PlaylistCollectionViewModel::setNewPlaylistName(QString name)
{
    if (m_newPlaylistName == name) {
        return;
    }

    m_newPlaylistName = std::move(name);
    emit newPlaylistNameChanged();
}

const QString &PlaylistCollectionViewModel::visiblePlaylistId() const noexcept
{
    return m_document.visiblePlaylistId();
}

const QString &PlaylistCollectionViewModel::visiblePlaylistName() const noexcept
{
    static const QString empty;
    const Model::LibraryPlaylist *playlist = m_document.visiblePlaylist();
    return playlist == nullptr ? empty : playlist->name();
}

int PlaylistCollectionViewModel::playlistCount() const noexcept
{
    return m_document.playlistCount();
}

const QString &PlaylistCollectionViewModel::selectedPlaylistId() const noexcept
{
    return m_selectedPlaylistId;
}

void PlaylistCollectionViewModel::setSelectedPlaylistId(QString playlistId)
{
    if (m_selectedPlaylistId == playlistId) {
        return;
    }

    m_selectedPlaylistId = std::move(playlistId);
    emit selectedPlaylistChanged();
}

int PlaylistCollectionViewModel::selectedSongIndex() const noexcept
{
    return m_selectedSongIndex;
}

void PlaylistCollectionViewModel::setSelectedSongIndex(int index)
{
    if (m_selectedSongIndex == index) {
        return;
    }

    m_selectedSongIndex = index;
    emit selectedSongIndexChanged();
}

const QString &PlaylistCollectionViewModel::lastError() const noexcept
{
    return m_lastError;
}

const QString &PlaylistCollectionViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
}

Common::ViewCommand *PlaylistCollectionViewModel::loadCommand() noexcept
{
    return &m_loadCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::saveCommand() noexcept
{
    return &m_saveCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::createPlaylistCommand() noexcept
{
    return &m_createPlaylistCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::deletePlaylistCommand() noexcept
{
    return &m_deletePlaylistCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::switchPlaylistCommand() noexcept
{
    return &m_switchPlaylistCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::addSongsCommand() noexcept
{
    return &m_addSongsCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::playSelectedSongCommand() noexcept
{
    return &m_playSelectedSongCommand;
}

Common::ViewCommand *PlaylistCollectionViewModel::playVisiblePlaylistCommand() noexcept
{
    return &m_playVisiblePlaylistCommand;
}

void PlaylistCollectionViewModel::setCurrentVisibleSongs(QVector<Model::AudioFile> songs)
{
    m_currentVisibleSongs = std::move(songs);
}

QVector<Model::AudioFile> PlaylistCollectionViewModel::currentVisibleSongsSnapshot() const
{
    return m_currentVisibleSongs;
}

bool PlaylistCollectionViewModel::executeLoad()
{
    applyResult(m_useCase->load(m_storagePath));
    return m_lastError.isEmpty();
}

bool PlaylistCollectionViewModel::executeSave()
{
    applyResult(m_useCase->save(m_storagePath, m_document));
    return m_lastError.isEmpty();
}

bool PlaylistCollectionViewModel::executeCreatePlaylist()
{
    const QString name = m_newPlaylistName.trimmed();
    if (name.isEmpty()) {
        setLastError(QStringLiteral("Playlist name is empty."));
        setStatusMessage(QStringLiteral("Create playlist failed."));
        return false;
    }

    applyResult(m_useCase->createPlaylist(m_document, nextPlaylistId(), name, QDateTime::currentDateTimeUtc()));
    return m_lastError.isEmpty();
}

bool PlaylistCollectionViewModel::executeDeletePlaylist()
{
    const QString playlistId = m_selectedPlaylistId.isEmpty() ? m_document.visiblePlaylistId() : m_selectedPlaylistId;
    applyResult(m_useCase->deletePlaylist(m_document, playlistId, QDateTime::currentDateTimeUtc()));
    return m_lastError.isEmpty();
}

bool PlaylistCollectionViewModel::executeSwitchPlaylist()
{
    const QString playlistId = m_selectedPlaylistId.isEmpty() ? m_document.visiblePlaylistId() : m_selectedPlaylistId;
    applyResult(m_useCase->switchVisiblePlaylist(m_document, playlistId));
    return m_lastError.isEmpty();
}

bool PlaylistCollectionViewModel::executeAddSongs()
{
    const QString playlistId = m_document.visiblePlaylistId();
    Model::LibraryPlaylist *playlist = m_document.visiblePlaylist();
    if (playlist == nullptr) {
        setLastError(QStringLiteral("No visible playlist."));
        setStatusMessage(QStringLiteral("Add songs failed."));
        return false;
    }

    bool allOk = true;
    QString lastStatus;
    Model::LibraryDocument updatedDocument = m_document;
    for (const Model::AudioFile &song : m_currentVisibleSongs) {
        ModelService::PlaylistCollectionResult result = m_useCase->addSongToPlaylist(updatedDocument,
                                                                                     playlistId,
                                                                                     song,
                                                                                     QDateTime::currentDateTimeUtc());
        if (!result.ok()) {
            allOk = false;
            setLastError(result.error);
            break;
        }
        updatedDocument = std::move(result.document);
        lastStatus = std::move(result.status);
    }

    if (allOk) {
        applyResult({std::move(updatedDocument), lastStatus, {}});
    }
    return allOk;
}

bool PlaylistCollectionViewModel::executePlaySelectedSong()
{
    if (m_selectedSongIndex < 0 || m_selectedSongIndex >= m_currentVisibleSongs.size()) {
        setLastError(QStringLiteral("Selected song index is invalid."));
        setStatusMessage(QStringLiteral("Play selected song failed."));
        return false;
    }

    setLastError({});
    setStatusMessage(QStringLiteral("Requested playback."));
    emit playRequested(m_document.visiblePlaylistId(), m_currentVisibleSongs, m_selectedSongIndex);
    return true;
}

bool PlaylistCollectionViewModel::executePlayVisiblePlaylist()
{
    if (m_currentVisibleSongs.isEmpty()) {
        setLastError(QStringLiteral("Visible playlist is empty."));
        setStatusMessage(QStringLiteral("Play playlist failed."));
        return false;
    }

    setLastError({});
    setStatusMessage(QStringLiteral("Requested playback."));
    emit playRequested(m_document.visiblePlaylistId(), m_currentVisibleSongs, 0);
    return true;
}

void PlaylistCollectionViewModel::applyResult(ModelService::PlaylistCollectionResult result)
{
    if (!result.ok()) {
        setLastError(std::move(result.error));
        if (!result.status.isEmpty()) {
            setStatusMessage(std::move(result.status));
        }
        return;
    }

    setDocument(std::move(result.document));
    setLastError({});
    setStatusMessage(std::move(result.status));
}

void PlaylistCollectionViewModel::setDocument(Model::LibraryDocument document)
{
    const QString previousVisiblePlaylistId = m_document.visiblePlaylistId();
    const int previousPlaylistCount = m_document.playlistCount();
    m_document = std::move(document);
    m_playlists.setPlaylists(m_document.playlists());
    if (m_selectedPlaylistId.isEmpty() || !m_document.containsPlaylistId(m_selectedPlaylistId)) {
        setSelectedPlaylistId(m_document.visiblePlaylistId());
    }
    if (previousPlaylistCount != m_document.playlistCount()) {
        emit playlistCountChanged();
    }
    if (previousVisiblePlaylistId != m_document.visiblePlaylistId()) {
        emit visiblePlaylistChanged();
        emitVisibleSongsChanged();
    }
}

void PlaylistCollectionViewModel::emitVisibleSongsChanged()
{
    const Model::LibraryPlaylist *playlist = m_document.visiblePlaylist();
    emit currentVisibleSongsChanged(playlist == nullptr ? QVector<Model::AudioFile>{} : playlist->songs());
}

void PlaylistCollectionViewModel::setLastError(QString error)
{
    if (m_lastError == error) {
        return;
    }

    m_lastError = std::move(error);
    emit lastErrorChanged();
}

void PlaylistCollectionViewModel::setStatusMessage(QString status)
{
    if (m_statusMessage == status) {
        return;
    }

    m_statusMessage = std::move(status);
    emit statusMessageChanged();
}

QString PlaylistCollectionViewModel::nextPlaylistId() const
{
    return QStringLiteral("playlist-%1").arg(m_document.playlistCount() + 1);
}

} // namespace AudioPlayer::ViewModel
