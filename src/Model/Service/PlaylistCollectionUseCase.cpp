#include "Model/Service/PlaylistCollectionUseCase.h"

#include "Model/JsonLibraryDocumentRepository.h"

#include <utility>

namespace AudioPlayer::Model::Service {

bool PlaylistCollectionResult::ok() const noexcept
{
    return error.isEmpty();
}

PlaylistCollectionResult PlaylistCollectionUseCase::createEmptyDocument(QString playlistId,
                                                                         const QDateTime &timestamp,
                                                                         QString playlistName) const
{
    return success(LibraryDocument::createEmpty(std::move(playlistId), timestamp, std::move(playlistName)),
                   QStringLiteral("Created empty playlist library."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::load(const QString &storagePath) const
{
    JsonLibraryDocumentRepository repository(storagePath);
    JsonLibraryDocumentRepository::LoadResult loadResult = repository.load();
    if (!loadResult.document.has_value()) {
        PlaylistCollectionResult result;
        result.error = loadResult.message;
        return result;
    }

    return success(std::move(*loadResult.document));
}

PlaylistCollectionResult PlaylistCollectionUseCase::save(const QString &storagePath,
                                                         const LibraryDocument &document) const
{
    JsonLibraryDocumentRepository repository(storagePath);
    JsonLibraryDocumentRepository::SaveResult saveResult = repository.save(document);
    if (!saveResult.saved) {
        return failure(document, saveResult.message);
    }

    return success(document, QStringLiteral("Saved playlist library."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::replaceWithEmptyV2(
    const QString &storagePath,
    const LibraryDocument &document) const
{
    JsonLibraryDocumentRepository repository(storagePath);
    JsonLibraryDocumentRepository::SaveResult saveResult = repository.replaceWithEmptyV2(document);
    if (!saveResult.saved) {
        return failure(document, saveResult.message);
    }

    return success(document, QStringLiteral("Replaced playlist library with schemaVersion 2 document."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::createPlaylist(const LibraryDocument &document,
                                                                   QString playlistId,
                                                                   QString name,
                                                                   const QDateTime &timestamp) const
{
    LibraryDocument updatedDocument = document;
    LibraryPlaylist playlist(std::move(playlistId), std::move(name), timestamp, timestamp);
    if (!updatedDocument.addPlaylist(playlist)) {
        return failure(document, QStringLiteral("Playlist id or name already exists, or playlist is invalid."));
    }

    return success(std::move(updatedDocument), QStringLiteral("Created playlist."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::switchVisiblePlaylist(const LibraryDocument &document,
                                                                          const QString &playlistId) const
{
    LibraryDocument updatedDocument = document;
    if (!updatedDocument.setVisiblePlaylistId(playlistId)) {
        return failure(document, QStringLiteral("Playlist does not exist."));
    }

    return success(std::move(updatedDocument), QStringLiteral("Switched visible playlist."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::deletePlaylist(const LibraryDocument &document,
                                                                   const QString &playlistId,
                                                                   const QDateTime &timestamp) const
{
    LibraryDocument updatedDocument = document;
    if (!updatedDocument.removePlaylistById(playlistId, timestamp)) {
        return failure(document, QStringLiteral("Playlist does not exist."));
    }

    return success(std::move(updatedDocument), QStringLiteral("Deleted playlist."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::addSongToPlaylist(const LibraryDocument &document,
                                                                      const QString &playlistId,
                                                                      const AudioFile &song,
                                                                      const QDateTime &timestamp) const
{
    LibraryDocument updatedDocument = document;
    LibraryPlaylist *playlist = updatedDocument.playlistAt(updatedDocument.findPlaylistById(playlistId));
    if (playlist == nullptr) {
        return failure(document, QStringLiteral("Playlist does not exist."));
    }

    if (!song.isValid()) {
        return failure(document, QStringLiteral("Cannot add a song with an empty file path."));
    }

    if (!playlist->addSong(song, timestamp)) {
        return success(std::move(updatedDocument), QStringLiteral("Song already exists in playlist."));
    }

    return success(std::move(updatedDocument), QStringLiteral("Added song to playlist."));
}

PlaylistCollectionResult PlaylistCollectionUseCase::success(LibraryDocument document, QString status) const
{
    PlaylistCollectionResult result;
    result.document = std::move(document);
    result.status = std::move(status);
    return result;
}

PlaylistCollectionResult PlaylistCollectionUseCase::failure(const LibraryDocument &document, QString error) const
{
    PlaylistCollectionResult result;
    result.document = document;
    result.error = std::move(error);
    return result;
}

} // namespace AudioPlayer::Model::Service
