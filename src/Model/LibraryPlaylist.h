#pragma once

#include "Model/AudioFile.h"

#include <QDateTime>
#include <QString>
#include <QVector>

namespace AudioPlayer::Model {

class LibraryPlaylist
{
public:
    LibraryPlaylist() = default;
    LibraryPlaylist(QString id, QString name, QDateTime createdAt, QDateTime updatedAt = {});

    [[nodiscard]] const QString &id() const noexcept;
    void setId(QString id);

    [[nodiscard]] const QString &name() const noexcept;
    void setName(QString name, const QDateTime &updatedAt = {});

    [[nodiscard]] const QVector<AudioFile> &songs() const noexcept;
    void setSongs(QVector<AudioFile> songs, const QDateTime &updatedAt = {});

    [[nodiscard]] const QDateTime &createdAt() const noexcept;
    void setCreatedAt(QDateTime createdAt);

    [[nodiscard]] const QDateTime &updatedAt() const noexcept;
    void setUpdatedAt(QDateTime updatedAt);

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] int size() const noexcept;

    [[nodiscard]] bool containsSongPath(const QString &filePath) const;
    [[nodiscard]] int findSongByPath(const QString &filePath) const;
    bool addSong(const AudioFile &song, const QDateTime &updatedAt = {});
    bool removeSongAt(int index, const QDateTime &updatedAt = {});

    [[nodiscard]] static QString normalizedSongPath(const QString &filePath);

private:
    void touch(const QDateTime &updatedAt);

    QString m_id;
    QString m_name;
    QVector<AudioFile> m_songs;
    QDateTime m_createdAt;
    QDateTime m_updatedAt;
};

} // namespace AudioPlayer::Model
