#include "Model/LibraryPlaylist.h"

#include <utility>

#include <QDir>
#include <QFileInfo>

namespace AudioPlayer::Model {

LibraryPlaylist::LibraryPlaylist(QString id, QString name, QDateTime createdAt, QDateTime updatedAt)
    : m_id(std::move(id))
    , m_name(std::move(name))
    , m_createdAt(std::move(createdAt))
    , m_updatedAt(updatedAt.isValid() ? std::move(updatedAt) : m_createdAt)
{
}

const QString &LibraryPlaylist::id() const noexcept
{
    return m_id;
}

void LibraryPlaylist::setId(QString id)
{
    m_id = std::move(id);
}

const QString &LibraryPlaylist::name() const noexcept
{
    return m_name;
}

void LibraryPlaylist::setName(QString name, const QDateTime &updatedAt)
{
    m_name = std::move(name);
    touch(updatedAt);
}

const QVector<AudioFile> &LibraryPlaylist::songs() const noexcept
{
    return m_songs;
}

void LibraryPlaylist::setSongs(QVector<AudioFile> songs, const QDateTime &updatedAt)
{
    m_songs = std::move(songs);
    touch(updatedAt);
}

const QDateTime &LibraryPlaylist::createdAt() const noexcept
{
    return m_createdAt;
}

void LibraryPlaylist::setCreatedAt(QDateTime createdAt)
{
    m_createdAt = std::move(createdAt);
}

const QDateTime &LibraryPlaylist::updatedAt() const noexcept
{
    return m_updatedAt;
}

void LibraryPlaylist::setUpdatedAt(QDateTime updatedAt)
{
    m_updatedAt = std::move(updatedAt);
}

bool LibraryPlaylist::isValid() const noexcept
{
    return !m_id.trimmed().isEmpty() && !m_name.trimmed().isEmpty() && m_createdAt.isValid()
        && m_updatedAt.isValid();
}

bool LibraryPlaylist::isEmpty() const noexcept
{
    return m_songs.isEmpty();
}

int LibraryPlaylist::size() const noexcept
{
    return m_songs.size();
}

bool LibraryPlaylist::containsSongPath(const QString &filePath) const
{
    return findSongByPath(filePath) >= 0;
}

int LibraryPlaylist::findSongByPath(const QString &filePath) const
{
    const QString normalizedPath = normalizedSongPath(filePath);
    if (normalizedPath.isEmpty()) {
        return -1;
    }

    for (int index = 0; index < m_songs.size(); ++index) {
        if (normalizedSongPath(m_songs.at(index).filePath()) == normalizedPath) {
            return index;
        }
    }

    return -1;
}

bool LibraryPlaylist::addSong(const AudioFile &song, const QDateTime &updatedAt)
{
    if (!song.isValid() || containsSongPath(song.filePath())) {
        return false;
    }

    m_songs.append(song);
    touch(updatedAt);
    return true;
}

bool LibraryPlaylist::removeSongAt(int index, const QDateTime &updatedAt)
{
    if (index < 0 || index >= m_songs.size()) {
        return false;
    }

    m_songs.removeAt(index);
    touch(updatedAt);
    return true;
}

QString LibraryPlaylist::normalizedSongPath(const QString &filePath)
{
    const QString trimmedPath = filePath.trimmed();
    if (trimmedPath.isEmpty()) {
        return {};
    }

    const QFileInfo fileInfo(trimmedPath);
    const QString canonicalPath = fileInfo.canonicalFilePath();
    if (!canonicalPath.isEmpty()) {
        return QDir::cleanPath(canonicalPath);
    }

    const QString absolutePath = fileInfo.absoluteFilePath();
    if (!absolutePath.isEmpty()) {
        return QDir::cleanPath(absolutePath);
    }

    return QDir::cleanPath(trimmedPath);
}

void LibraryPlaylist::touch(const QDateTime &updatedAt)
{
    if (updatedAt.isValid()) {
        m_updatedAt = updatedAt;
    }
}

} // namespace AudioPlayer::Model
