#include "Model/PlayList.h"

#include <algorithm>
#include <utility>

namespace AudioPlayer::Model {

bool PlayList::isEmpty() const noexcept
{
    return m_items.isEmpty();
}

int PlayList::size() const noexcept
{
    return m_items.size();
}

const QVector<AudioFile> &PlayList::items() const noexcept
{
    return m_items;
}

void PlayList::add(AudioFile audioFile)
{
    m_items.push_back(std::move(audioFile));
    if (m_currentIndex == -1) {
        m_currentIndex = 0;
    }
}

bool PlayList::insert(int index, AudioFile audioFile)
{
    if (index < 0 || index > m_items.size()) {
        return false;
    }

    m_items.insert(index, std::move(audioFile));
    if (m_currentIndex == -1) {
        m_currentIndex = 0;
    } else if (index <= m_currentIndex) {
        ++m_currentIndex;
    }
    return true;
}

bool PlayList::removeAt(int index)
{
    if (!isValidIndex(index)) {
        return false;
    }

    m_items.removeAt(index);
    if (m_items.isEmpty()) {
        m_currentIndex = -1;
    } else if (index < m_currentIndex) {
        --m_currentIndex;
    } else if (index == m_currentIndex && m_currentIndex >= m_items.size()) {
        m_currentIndex = m_items.size() - 1;
    }
    return true;
}

void PlayList::clear() noexcept
{
    m_items.clear();
    m_currentIndex = -1;
}

bool PlayList::move(int fromIndex, int toIndex)
{
    if (!isValidIndex(fromIndex) || !isValidIndex(toIndex)) {
        return false;
    }
    if (fromIndex == toIndex) {
        return true;
    }

    m_items.move(fromIndex, toIndex);
    if (m_currentIndex == fromIndex) {
        m_currentIndex = toIndex;
    } else if (fromIndex < m_currentIndex && toIndex >= m_currentIndex) {
        --m_currentIndex;
    } else if (fromIndex > m_currentIndex && toIndex <= m_currentIndex) {
        ++m_currentIndex;
    }
    return true;
}

const AudioFile *PlayList::at(int index) const noexcept
{
    return isValidIndex(index) ? &m_items[index] : nullptr;
}

AudioFile *PlayList::at(int index) noexcept
{
    return isValidIndex(index) ? &m_items[index] : nullptr;
}

int PlayList::findByPath(const QString &filePath) const noexcept
{
    const auto iterator = std::find_if(m_items.cbegin(), m_items.cend(), [&filePath](const AudioFile &item) {
        return item.filePath() == filePath;
    });

    if (iterator == m_items.cend()) {
        return -1;
    }

    return static_cast<int>(std::distance(m_items.cbegin(), iterator));
}

bool PlayList::containsPath(const QString &filePath) const noexcept
{
    return findByPath(filePath) != -1;
}

int PlayList::currentIndex() const noexcept
{
    return m_currentIndex;
}

bool PlayList::setCurrentIndex(int index) noexcept
{
    if (!isValidIndex(index)) {
        return false;
    }

    m_currentIndex = index;
    return true;
}

void PlayList::clearCurrentIndex() noexcept
{
    m_currentIndex = -1;
}

std::optional<AudioFile> PlayList::currentFile() const
{
    if (!isValidIndex(m_currentIndex)) {
        return std::nullopt;
    }

    return m_items[m_currentIndex];
}

qint64 PlayList::totalDurationSeconds() const noexcept
{
    qint64 total = 0;
    for (const AudioFile &item : m_items) {
        total += item.durationSeconds();
    }
    return total;
}

bool PlayList::isValidIndex(int index) const noexcept
{
    return index >= 0 && index < m_items.size();
}

} // namespace AudioPlayer::Model
