#include "Model/Service/PlaybackUseCase.h"

#include <QFileInfo>

#include <algorithm>
#include <utility>

namespace AudioPlayer::Model::Service {
namespace {

QString missingOrInvalidMessage(const AudioFile &audioFile)
{
    const QString displayName = audioFile.displayTitle();
    if (!audioFile.isValid()) {
        return QStringLiteral("Cannot play an empty audio file path.");
    }

    return QStringLiteral("Cannot play missing or inaccessible file: %1").arg(displayName);
}

} // namespace

PlaybackUseCase::PlaybackUseCase(IPlaybackService &playbackService, QObject *parent)
    : QObject(parent)
    , m_playbackService(playbackService)
{
    connect(&m_playbackService, &IPlaybackService::playbackEnded, this, &PlaybackUseCase::handlePlaybackEnded);
    connect(&m_playbackService, &IPlaybackService::playbackError, this, &PlaybackUseCase::handlePlaybackError);
    connect(&m_playbackService, &IPlaybackService::stateChanged, this, &PlaybackUseCase::playbackStateChanged);
    connect(&m_playbackService, &IPlaybackService::positionChanged, this, &PlaybackUseCase::positionChanged);
    connect(&m_playbackService, &IPlaybackService::durationChanged, this, &PlaybackUseCase::durationChanged);
    connect(&m_playbackService, &IPlaybackService::seekableChanged, this, &PlaybackUseCase::seekableChanged);
    connect(&m_playbackService, &IPlaybackService::volumeChanged, this, &PlaybackUseCase::volumeChanged);
    connect(&m_playbackService, &IPlaybackService::mutedChanged, this, &PlaybackUseCase::mutedChanged);
}

int PlaybackUseCase::currentIndex() const noexcept
{
    return m_currentIndex;
}

const QVector<AudioFile> &PlaybackUseCase::queue() const noexcept
{
    return m_queue;
}

std::optional<AudioFile> PlaybackUseCase::currentTrack() const
{
    if (!isValidIndex(m_currentIndex)) {
        return std::nullopt;
    }

    return m_queue.at(m_currentIndex);
}

bool PlaybackUseCase::hasQueue() const noexcept
{
    return !m_queue.isEmpty();
}

PlaybackBackendState PlaybackUseCase::playbackState() const noexcept
{
    return m_playbackService.state();
}

qint64 PlaybackUseCase::positionMs() const noexcept
{
    return m_playbackService.positionMs();
}

qint64 PlaybackUseCase::durationMs() const noexcept
{
    return m_playbackService.durationMs();
}

bool PlaybackUseCase::seekable() const noexcept
{
    return m_playbackService.seekable();
}

float PlaybackUseCase::volume() const noexcept
{
    return m_playbackService.volume();
}

bool PlaybackUseCase::muted() const noexcept
{
    return m_playbackService.muted();
}

void PlaybackUseCase::setQueue(QVector<AudioFile> queue)
{
    const QString currentSource = m_playbackService.source();
    m_queue = std::move(queue);
    emit queueChanged();

    if (m_queue.isEmpty()) {
        m_playbackService.stop();
        m_playbackService.setSource({});
        setCurrentIndexInternal(-1);
        emit playbackStopped();
        return;
    }

    const int replacementIndex = currentSource.isEmpty() ? -1 : findByPath(currentSource);
    if (replacementIndex >= 0) {
        setCurrentIndexInternal(replacementIndex);
        return;
    }

    if (!currentSource.isEmpty()) {
        m_playbackService.stop();
        m_playbackService.setSource({});
        setCurrentIndexInternal(-1);
        emit playbackStopped();
        return;
    }

    if (!isValidIndex(m_currentIndex)) {
        setCurrentIndexInternal(-1);
    }
}

bool PlaybackUseCase::setCurrentIndex(int index)
{
    if (!isValidIndex(index)) {
        return false;
    }

    setCurrentIndexInternal(index);
    return true;
}

void PlaybackUseCase::play()
{
    if (m_queue.isEmpty()) {
        stopAtQueueEnd(QStringLiteral("No songs are available to play."));
        return;
    }

    const int startIndex = isValidIndex(m_currentIndex) ? m_currentIndex : 0;
    if (!startAtOrSkip(startIndex, Direction::Forward)) {
        stopAtQueueEnd(QStringLiteral("No playable songs are available."));
    }
}

void PlaybackUseCase::pause()
{
    m_playbackService.pause();
}

void PlaybackUseCase::stop()
{
    m_playbackService.stop();
    emit playbackStopped();
}

void PlaybackUseCase::next()
{
    if (m_queue.isEmpty()) {
        stopAtQueueEnd(QStringLiteral("No songs are available to play."));
        return;
    }

    const int startIndex = isValidIndex(m_currentIndex) ? m_currentIndex + 1 : 0;
    if (!startAtOrSkip(startIndex, Direction::Forward)) {
        stopAtQueueEnd(QStringLiteral("Reached the end of the playback queue."));
    }
}

void PlaybackUseCase::previous()
{
    if (m_queue.isEmpty()) {
        stopAtQueueEnd(QStringLiteral("No songs are available to play."));
        return;
    }

    const int startIndex = isValidIndex(m_currentIndex) ? std::max(0, m_currentIndex - 1) : 0;
    if (!startAtOrSkip(startIndex, Direction::Backward)) {
        stopAtQueueEnd(QStringLiteral("No playable previous song is available."));
    }
}

void PlaybackUseCase::seekTo(qint64 positionMs)
{
    if (!m_playbackService.seekable()) {
        return;
    }

    m_playbackService.seekTo(positionMs);
}

void PlaybackUseCase::setVolume(float volume)
{
    m_playbackService.setVolume(volume);
}

void PlaybackUseCase::setMuted(bool muted)
{
    m_playbackService.setMuted(muted);
}

void PlaybackUseCase::handlePlaybackEnded()
{
    next();
}

void PlaybackUseCase::handlePlaybackError(PlaybackBackendError error, const QString &message)
{
    Q_UNUSED(error)
    reportError(message.isEmpty() ? QStringLiteral("Playback failed.") : message);

    const int startIndex = isValidIndex(m_currentIndex) ? m_currentIndex + 1 : 0;
    if (!startAtOrSkip(startIndex, Direction::Forward)) {
        stopAtQueueEnd(QStringLiteral("No playable songs remain after playback error."));
    }
}

bool PlaybackUseCase::isValidIndex(int index) const noexcept
{
    return index >= 0 && index < m_queue.size();
}

bool PlaybackUseCase::isPlayableCandidate(const AudioFile &audioFile, QString *errorMessage) const
{
    if (!audioFile.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = missingOrInvalidMessage(audioFile);
        }
        return false;
    }

    const QFileInfo fileInfo(audioFile.filePath());
    if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable()) {
        if (errorMessage != nullptr) {
            *errorMessage = missingOrInvalidMessage(audioFile);
        }
        return false;
    }

    return true;
}

int PlaybackUseCase::findByPath(const QString &filePath) const noexcept
{
    for (int index = 0; index < m_queue.size(); ++index) {
        if (m_queue.at(index).filePath() == filePath) {
            return index;
        }
    }

    return -1;
}

void PlaybackUseCase::setCurrentIndexInternal(int index)
{
    if (m_currentIndex == index) {
        return;
    }

    m_currentIndex = index;
    emit currentIndexChanged(m_currentIndex);
    emitCurrentTrackChanged();
}

void PlaybackUseCase::emitCurrentTrackChanged()
{
    if (!isValidIndex(m_currentIndex)) {
        emit currentTrackChanged(-1, {}, {});
        return;
    }

    const AudioFile &audioFile = m_queue.at(m_currentIndex);
    emit currentTrackChanged(m_currentIndex, audioFile.displayTitle(), audioFile.filePath());
}

void PlaybackUseCase::reportError(QString message)
{
    emit playbackErrorOccurred(std::move(message));
}

void PlaybackUseCase::stopAtQueueEnd(const QString &message)
{
    m_playbackService.stop();
    if (!message.isEmpty()) {
        reportError(message);
    }
    emit playbackStopped();
}

bool PlaybackUseCase::startAtOrSkip(int startIndex, Direction direction)
{
    if (m_queue.isEmpty()) {
        return false;
    }

    int index = startIndex;
    int visited = 0;
    while (isValidIndex(index) && visited < m_queue.size()) {
        QString errorMessage;
        if (isPlayableCandidate(m_queue.at(index), &errorMessage)) {
            return startPlaybackAt(index);
        }

        reportError(errorMessage);
        index += direction == Direction::Forward ? 1 : -1;
        ++visited;
    }

    return false;
}

bool PlaybackUseCase::startPlaybackAt(int index)
{
    if (!isValidIndex(index)) {
        return false;
    }

    const AudioFile &audioFile = m_queue.at(index);
    setCurrentIndexInternal(index);
    m_playbackService.setSource(audioFile.filePath());
    m_playbackService.play();
    return true;
}

} // namespace AudioPlayer::Model::Service
