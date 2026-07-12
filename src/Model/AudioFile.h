#pragma once

#include <QtGlobal>
#include <QString>

namespace AudioPlayer::Model {

class AudioFile
{
public:
    AudioFile() = default;
    explicit AudioFile(QString filePath);
    AudioFile(QString filePath,
              QString title,
              QString artist = {},
              QString album = {},
              qint64 durationSeconds = 0);

    [[nodiscard]] const QString &filePath() const noexcept;
    void setFilePath(QString filePath);

    [[nodiscard]] const QString &title() const noexcept;
    void setTitle(QString title);

    [[nodiscard]] const QString &artist() const noexcept;
    void setArtist(QString artist);

    [[nodiscard]] const QString &album() const noexcept;
    void setAlbum(QString album);

    [[nodiscard]] qint64 durationSeconds() const noexcept;
    [[nodiscard]] QString extension() const;
    void setDurationSeconds(qint64 durationSeconds) noexcept;

    [[nodiscard]] bool isValid() const;
    [[nodiscard]] QString displayTitle() const;

    friend bool operator==(const AudioFile &left, const AudioFile &right) noexcept;
    friend bool operator!=(const AudioFile &left, const AudioFile &right) noexcept;

private:
    QString m_filePath;
    QString m_title;
    QString m_artist;
    QString m_album;
    qint64 m_durationSeconds = 0;
};

} // namespace AudioPlayer::Model
