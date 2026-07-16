#pragma once

#include "Model/LibraryPlaylist.h"

#include <QString>
#include <QVector>

namespace AudioPlayer::Model {

class LibraryDocument
{
public:
    static constexpr int CurrentSchemaVersion = 2;
    LibraryDocument();
    LibraryDocument(QString visiblePlaylistId, QVector<LibraryPlaylist> playlists);

    [[nodiscard]] int schemaVersion() const noexcept;

    [[nodiscard]] const QString &visiblePlaylistId() const noexcept;
    bool setVisiblePlaylistId(const QString &playlistId);

    [[nodiscard]] const QVector<LibraryPlaylist> &playlists() const noexcept;
    void setPlaylists(QVector<LibraryPlaylist> playlists);

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] int playlistCount() const noexcept;

    [[nodiscard]] int findPlaylistById(const QString &playlistId) const noexcept;
    [[nodiscard]] int findPlaylistByName(const QString &name) const noexcept;
    [[nodiscard]] bool containsPlaylistId(const QString &playlistId) const noexcept;
    [[nodiscard]] bool containsPlaylistName(const QString &name) const noexcept;

    [[nodiscard]] const LibraryPlaylist *visiblePlaylist() const noexcept;
    [[nodiscard]] LibraryPlaylist *visiblePlaylist() noexcept;
    [[nodiscard]] const LibraryPlaylist *playlistAt(int index) const noexcept;
    [[nodiscard]] LibraryPlaylist *playlistAt(int index) noexcept;

    bool addPlaylist(const LibraryPlaylist &playlist);
    bool removePlaylistById(const QString &playlistId, const QDateTime &fallbackTimestamp = {});
    void ensureDefaultPlaylist(const QDateTime &timestamp = {});

    [[nodiscard]] static LibraryDocument createEmpty(QString playlistId,
                                                     const QDateTime &timestamp,
                                                     QString playlistName = QStringLiteral("Default"));

private:
    [[nodiscard]] bool isValidIndex(int index) const noexcept;
    void repairVisiblePlaylistId() noexcept;

    QString m_visiblePlaylistId;
    QVector<LibraryPlaylist> m_playlists;
};

} // namespace AudioPlayer::Model
