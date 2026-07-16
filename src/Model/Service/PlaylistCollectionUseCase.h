#pragma once

#include "Model/AudioFile.h"
#include "Model/LibraryDocument.h"

#include <QString>

namespace AudioPlayer::Model::Service {

struct PlaylistCollectionResult
{
    LibraryDocument document;
    QString status;
    QString error;

    [[nodiscard]] bool ok() const noexcept;
};

class PlaylistCollectionUseCase
{
public:
    [[nodiscard]] PlaylistCollectionResult createEmptyDocument(QString playlistId,
                                                               const QDateTime &timestamp,
                                                               QString playlistName = QStringLiteral("Default")) const;
    [[nodiscard]] PlaylistCollectionResult load(const QString &storagePath) const;
    [[nodiscard]] PlaylistCollectionResult save(const QString &storagePath,
                                                const LibraryDocument &document) const;
    [[nodiscard]] PlaylistCollectionResult replaceWithEmptyV2(const QString &storagePath,
                                                              const LibraryDocument &document) const;

    [[nodiscard]] PlaylistCollectionResult createPlaylist(const LibraryDocument &document,
                                                          QString playlistId,
                                                          QString name,
                                                          const QDateTime &timestamp) const;
    [[nodiscard]] PlaylistCollectionResult switchVisiblePlaylist(const LibraryDocument &document,
                                                                 const QString &playlistId) const;
    [[nodiscard]] PlaylistCollectionResult deletePlaylist(const LibraryDocument &document,
                                                          const QString &playlistId,
                                                          const QDateTime &timestamp) const;
    [[nodiscard]] PlaylistCollectionResult addSongToPlaylist(const LibraryDocument &document,
                                                             const QString &playlistId,
                                                             const AudioFile &song,
                                                             const QDateTime &timestamp) const;

private:
    [[nodiscard]] PlaylistCollectionResult success(LibraryDocument document, QString status = {}) const;
    [[nodiscard]] PlaylistCollectionResult failure(const LibraryDocument &document, QString error) const;
};

} // namespace AudioPlayer::Model::Service
