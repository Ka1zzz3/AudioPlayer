#include "Model/Service/TranscodedPlaylistService.h"

#include <utility>

namespace AudioPlayer::Model::Service {

TranscodedPlaylistService::TranscodedPlaylistService(QString storagePath, PlaylistCollectionUseCase useCase)
    : m_storagePath(std::move(storagePath))
    , m_useCase(std::move(useCase))
{
}

const QString &TranscodedPlaylistService::storagePath() const noexcept
{
    return m_storagePath;
}

void TranscodedPlaylistService::setStoragePath(QString storagePath)
{
    m_storagePath = std::move(storagePath);
}

TranscodedPlaylistResult TranscodedPlaylistService::addOutput(const AudioFile &song, const QDateTime &timestamp) const
{
    if (m_storagePath.trimmed().isEmpty()) {
        return {false, {}, QStringLiteral("Playlist storage path is empty.")};
    }
    if (!timestamp.isValid()) {
        return {false, {}, QStringLiteral("Invalid playlist update timestamp.")};
    }
    if (!song.isValid()) {
        return {false, {}, QStringLiteral("Cannot add transcoded output with an empty file path.")};
    }

    PlaylistCollectionResult loadResult = m_useCase.load(m_storagePath);
    if (!loadResult.ok()) {
        return {false, {}, loadResult.error};
    }

    LibraryDocument document = std::move(loadResult.document);
    const QString playlistId = QString::fromLatin1(TranscodedPlaylistId);
    if (!document.containsPlaylistId(playlistId)) {
        PlaylistCollectionResult createResult = m_useCase.createPlaylist(document,
                                                                        playlistId,
                                                                        QString::fromLatin1(TranscodedPlaylistName),
                                                                        timestamp);
        if (!createResult.ok()) {
            return {false, {}, createResult.error};
        }
        document = std::move(createResult.document);
    }

    PlaylistCollectionResult addResult = m_useCase.addSongToPlaylist(document, playlistId, song, timestamp);
    if (!addResult.ok()) {
        return {false, {}, addResult.error};
    }
    document = std::move(addResult.document);

    PlaylistCollectionResult saveResult = m_useCase.save(m_storagePath, document);
    if (!saveResult.ok()) {
        return {false, {}, saveResult.error};
    }

    return {true, QStringLiteral("Added transcoded output to Transcoded playlist."), {}};
}

} // namespace AudioPlayer::Model::Service
