#pragma once

#include "Model/AudioFile.h"

#include <optional>

#include <QVector>

namespace AudioPlayer::Model {

class PlayList
{
public:
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] int size() const noexcept;
    [[nodiscard]] const QVector<AudioFile> &items() const noexcept;

    void add(AudioFile audioFile);
    bool insert(int index, AudioFile audioFile);
    bool removeAt(int index);
    void clear() noexcept;
    bool move(int fromIndex, int toIndex);

    [[nodiscard]] const AudioFile *at(int index) const noexcept;
    [[nodiscard]] AudioFile *at(int index) noexcept;

    [[nodiscard]] int findByPath(const QString &filePath) const noexcept;
    [[nodiscard]] bool containsPath(const QString &filePath) const noexcept;

    [[nodiscard]] int currentIndex() const noexcept;
    bool setCurrentIndex(int index) noexcept;
    void clearCurrentIndex() noexcept;
    [[nodiscard]] std::optional<AudioFile> currentFile() const;

    [[nodiscard]] qint64 totalDurationSeconds() const noexcept;

private:
    [[nodiscard]] bool isValidIndex(int index) const noexcept;

    QVector<AudioFile> m_items;
    int m_currentIndex = -1;
};

} // namespace AudioPlayer::Model
