#include "Model/AudioFile.h"

#include <utility>

namespace AudioPlayer::Model {

AudioFile::AudioFile(QString filePath)
    : m_filePath(std::move(filePath))
{
}

AudioFile::AudioFile(QString filePath,
                     QString title,
                     QString artist,
                     QString album,
                     qint64 durationSeconds)
    : m_filePath(std::move(filePath))
    , m_title(std::move(title))
    , m_artist(std::move(artist))
    , m_album(std::move(album))
{
    setDurationSeconds(durationSeconds);
}

const QString &AudioFile::filePath() const noexcept
{
    return m_filePath;
}

void AudioFile::setFilePath(QString filePath)
{
    m_filePath = std::move(filePath);
}

const QString &AudioFile::title() const noexcept
{
    return m_title;
}

void AudioFile::setTitle(QString title)
{
    m_title = std::move(title);
}

const QString &AudioFile::artist() const noexcept
{
    return m_artist;
}

void AudioFile::setArtist(QString artist)
{
    m_artist = std::move(artist);
}

const QString &AudioFile::album() const noexcept
{
    return m_album;
}

void AudioFile::setAlbum(QString album)
{
    m_album = std::move(album);
}

qint64 AudioFile::durationSeconds() const noexcept
{
    return m_durationSeconds;
}

void AudioFile::setDurationSeconds(qint64 durationSeconds) noexcept
{
    m_durationSeconds = durationSeconds < 0 ? 0 : durationSeconds;
}

bool AudioFile::isValid() const
{
    return !m_filePath.trimmed().isEmpty();
}

QString AudioFile::displayTitle() const
{
    const QString trimmedTitle = m_title.trimmed();
    if (!trimmedTitle.isEmpty()) {
        return trimmedTitle;
    }

    return m_filePath;
}

bool operator==(const AudioFile &left, const AudioFile &right) noexcept
{
    return left.m_filePath == right.m_filePath
        && left.m_title == right.m_title
        && left.m_artist == right.m_artist
        && left.m_album == right.m_album
        && left.m_durationSeconds == right.m_durationSeconds;
}

bool operator!=(const AudioFile &left, const AudioFile &right) noexcept
{
    return !(left == right);
}

} // namespace AudioPlayer::Model
