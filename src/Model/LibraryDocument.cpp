#include "Model/LibraryDocument.h"

#include <utility>

namespace AudioPlayer::Model {

LibraryDocument::LibraryDocument() = default;

LibraryDocument::LibraryDocument(QString visiblePlaylistId, QVector<LibraryPlaylist> playlists)
    : m_visiblePlaylistId(std::move(visiblePlaylistId))
    , m_playlists(std::move(playlists))
{
    repairVisiblePlaylistId();
}

int LibraryDocument::schemaVersion() const noexcept
{
    return CurrentSchemaVersion;
}

const QString &LibraryDocument::visiblePlaylistId() const noexcept
{
    return m_visiblePlaylistId;
}

bool LibraryDocument::setVisiblePlaylistId(const QString &playlistId)
{
    if (!containsPlaylistId(playlistId)) {
        return false;
    }

    m_visiblePlaylistId = playlistId;
    return true;
}

const QVector<LibraryPlaylist> &LibraryDocument::playlists() const noexcept
{
    return m_playlists;
}

void LibraryDocument::setPlaylists(QVector<LibraryPlaylist> playlists)
{
    m_playlists = std::move(playlists);
    repairVisiblePlaylistId();
}

bool LibraryDocument::isValid() const noexcept
{
    if (m_playlists.isEmpty() || !containsPlaylistId(m_visiblePlaylistId)) {
        return false;
    }

    for (const LibraryPlaylist &playlist : m_playlists) {
        if (!playlist.isValid()) {
            return false;
        }
    }

    return true;
}

bool LibraryDocument::isEmpty() const noexcept
{
    return m_playlists.isEmpty();
}

int LibraryDocument::playlistCount() const noexcept
{
    return m_playlists.size();
}

int LibraryDocument::findPlaylistById(const QString &playlistId) const noexcept
{
    for (int index = 0; index < m_playlists.size(); ++index) {
        if (m_playlists.at(index).id() == playlistId) {
            return index;
        }
    }

    return -1;
}

int LibraryDocument::findPlaylistByName(const QString &name) const noexcept
{
    const QString trimmedName = name.trimmed();
    for (int index = 0; index < m_playlists.size(); ++index) {
        if (m_playlists.at(index).name().trimmed() == trimmedName) {
            return index;
        }
    }

    return -1;
}

bool LibraryDocument::containsPlaylistId(const QString &playlistId) const noexcept
{
    return findPlaylistById(playlistId) >= 0;
}

bool LibraryDocument::containsPlaylistName(const QString &name) const noexcept
{
    return findPlaylistByName(name) >= 0;
}

const LibraryPlaylist *LibraryDocument::visiblePlaylist() const noexcept
{
    return playlistAt(findPlaylistById(m_visiblePlaylistId));
}

LibraryPlaylist *LibraryDocument::visiblePlaylist() noexcept
{
    return playlistAt(findPlaylistById(m_visiblePlaylistId));
}

const LibraryPlaylist *LibraryDocument::playlistAt(int index) const noexcept
{
    if (!isValidIndex(index)) {
        return nullptr;
    }

    return &m_playlists.at(index);
}

LibraryPlaylist *LibraryDocument::playlistAt(int index) noexcept
{
    if (!isValidIndex(index)) {
        return nullptr;
    }

    return &m_playlists[index];
}

bool LibraryDocument::addPlaylist(const LibraryPlaylist &playlist)
{
    if (!playlist.isValid() || containsPlaylistId(playlist.id()) || containsPlaylistName(playlist.name())) {
        return false;
    }

    m_playlists.append(playlist);
    if (m_visiblePlaylistId.isEmpty()) {
        m_visiblePlaylistId = playlist.id();
    }
    return true;
}

bool LibraryDocument::removePlaylistById(const QString &playlistId, const QDateTime &fallbackTimestamp)
{
    const int removedIndex = findPlaylistById(playlistId);
    if (removedIndex < 0) {
        return false;
    }

    const bool removedVisiblePlaylist = m_visiblePlaylistId == playlistId;
    m_playlists.removeAt(removedIndex);

    if (m_playlists.isEmpty()) {
        ensureDefaultPlaylist(fallbackTimestamp);
        return true;
    }

    if (removedVisiblePlaylist) {
        const int nextVisibleIndex = removedIndex < m_playlists.size() ? removedIndex : m_playlists.size() - 1;
        m_visiblePlaylistId = m_playlists.at(nextVisibleIndex).id();
    }

    repairVisiblePlaylistId();
    return true;
}

void LibraryDocument::ensureDefaultPlaylist(const QDateTime &timestamp)
{
    if (!m_playlists.isEmpty()) {
        repairVisiblePlaylistId();
        return;
    }

    const QDateTime playlistTimestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTimeUtc();
    LibraryPlaylist defaultPlaylist(QStringLiteral("default"),
                                    QStringLiteral("Default"),
                                    playlistTimestamp,
                                    playlistTimestamp);
    m_playlists.append(defaultPlaylist);
    m_visiblePlaylistId = defaultPlaylist.id();
}

LibraryDocument LibraryDocument::createEmpty(QString playlistId,
                                             const QDateTime &timestamp,
                                             QString playlistName)
{
    LibraryPlaylist playlist(std::move(playlistId), std::move(playlistName), timestamp, timestamp);
    LibraryDocument document;
    document.addPlaylist(playlist);
    document.setVisiblePlaylistId(playlist.id());
    return document;
}

bool LibraryDocument::isValidIndex(int index) const noexcept
{
    return index >= 0 && index < m_playlists.size();
}

void LibraryDocument::repairVisiblePlaylistId() noexcept
{
    if (m_playlists.isEmpty()) {
        m_visiblePlaylistId.clear();
        return;
    }

    if (!containsPlaylistId(m_visiblePlaylistId)) {
        m_visiblePlaylistId = m_playlists.first().id();
    }
}

} // namespace AudioPlayer::Model
