#include "ViewModel/PlaybackViewModel.h"

#include "Model/AudioFile.h"
#include "Model/Service/PlaybackUseCase.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace AudioPlayer::ViewModel {
namespace {

PlaybackState toPlaybackState(ModelService::PlaybackBackendState state)
{
    switch (state) {
    case ModelService::PlaybackBackendState::Loading:
        return PlaybackState::Loading;
    case ModelService::PlaybackBackendState::Playing:
        return PlaybackState::Playing;
    case ModelService::PlaybackBackendState::Paused:
        return PlaybackState::Paused;
    case ModelService::PlaybackBackendState::Buffering:
        return PlaybackState::Buffering;
    case ModelService::PlaybackBackendState::Error:
        return PlaybackState::Error;
    case ModelService::PlaybackBackendState::Stopped:
    case ModelService::PlaybackBackendState::Ended:
        return PlaybackState::Stopped;
    }

    return PlaybackState::Stopped;
}

int toVolumePercent(float volume)
{
    const float clamped = std::clamp(volume, 0.0F, 1.0F);
    return std::clamp(static_cast<int>(std::lround(clamped * 100.0F)), 0, 100);
}

float toBackendVolume(int volumePercent)
{
    return static_cast<float>(std::clamp(volumePercent, 0, 100)) / 100.0F;
}

} // namespace

PlaybackViewModel::PlaybackViewModel(ModelService::PlaybackUseCase &playbackUseCase, QObject *parent)
    : PlaybackViewModelProtocol(parent)
    , m_playbackUseCase(playbackUseCase)
    , m_playCommand(QStringLiteral("play"), [this]() { return executePlay(); }, this)
    , m_pauseCommand(QStringLiteral("pause"), [this]() { return executePause(); }, this)
    , m_stopCommand(QStringLiteral("stop"), [this]() { return executeStop(); }, this)
    , m_previousCommand(QStringLiteral("previous"), [this]() { return executePrevious(); }, this)
    , m_nextCommand(QStringLiteral("next"), [this]() { return executeNext(); }, this)
    , m_toggleMuteCommand(QStringLiteral("toggleMute"), [this]() { return executeToggleMute(); }, this)
{
    m_playbackState = toPlaybackState(m_playbackUseCase.playbackState());
    m_positionMs = m_playbackUseCase.positionMs();
    m_durationMs = m_playbackUseCase.durationMs();
    m_seekable = m_playbackUseCase.seekable();
    m_volumePercent = toVolumePercent(m_playbackUseCase.volume());
    m_muted = m_playbackUseCase.muted();

    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::playbackStateChanged, this, &PlaybackViewModel::updatePlaybackState);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::positionChanged, this, &PlaybackViewModel::updatePosition);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::durationChanged, this, &PlaybackViewModel::updateDuration);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::seekableChanged, this, &PlaybackViewModel::updateSeekable);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::volumeChanged, this, &PlaybackViewModel::updateVolume);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::mutedChanged, this, &PlaybackViewModel::updateMuted);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::currentTrackChanged, this, &PlaybackViewModel::updateCurrentTrack);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::playbackErrorOccurred, this, &PlaybackViewModel::updatePlaybackError);
    connect(&m_playbackUseCase, &ModelService::PlaybackUseCase::playbackStopped, this, [this]() {
        updateStatusMessage(QStringLiteral("Playback stopped."));
    });
}

PlaybackState PlaybackViewModel::playbackState() const noexcept
{
    return m_playbackState;
}

int PlaybackViewModel::currentPlaybackIndex() const noexcept
{
    return m_currentPlaybackIndex;
}

const QString &PlaybackViewModel::currentPlaybackTitle() const noexcept
{
    return m_currentPlaybackTitle;
}

qint64 PlaybackViewModel::positionMs() const noexcept
{
    return m_positionMs;
}

qint64 PlaybackViewModel::durationMs() const noexcept
{
    return m_durationMs;
}

bool PlaybackViewModel::seekable() const noexcept
{
    return m_seekable;
}

int PlaybackViewModel::volumePercent() const noexcept
{
    return m_volumePercent;
}

bool PlaybackViewModel::muted() const noexcept
{
    return m_muted;
}

const QString &PlaybackViewModel::lastPlaybackError() const noexcept
{
    return m_lastPlaybackError;
}

const QString &PlaybackViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
}

Common::ViewCommand *PlaybackViewModel::playCommand() noexcept
{
    return &m_playCommand;
}

Common::ViewCommand *PlaybackViewModel::pauseCommand() noexcept
{
    return &m_pauseCommand;
}

Common::ViewCommand *PlaybackViewModel::stopCommand() noexcept
{
    return &m_stopCommand;
}

Common::ViewCommand *PlaybackViewModel::previousCommand() noexcept
{
    return &m_previousCommand;
}

Common::ViewCommand *PlaybackViewModel::nextCommand() noexcept
{
    return &m_nextCommand;
}

Common::ViewCommand *PlaybackViewModel::toggleMuteCommand() noexcept
{
    return &m_toggleMuteCommand;
}

void PlaybackViewModel::replaceQueue(QVector<Model::AudioFile> queue)
{
    m_playbackUseCase.setQueue(std::move(queue));
}

void PlaybackViewModel::setCurrentPlaybackIndex(int index)
{
    m_playbackUseCase.setCurrentIndex(index);
}

void PlaybackViewModel::seekTo(qint64 positionMs)
{
    m_playbackUseCase.seekTo(positionMs);
}

void PlaybackViewModel::setVolumePercent(int volumePercent)
{
    m_playbackUseCase.setVolume(toBackendVolume(volumePercent));
}

void PlaybackViewModel::setMuted(bool muted)
{
    m_playbackUseCase.setMuted(muted);
}

bool PlaybackViewModel::executePlay()
{
    clearPlaybackError();
    m_playbackUseCase.play();
    if (m_lastPlaybackError.isEmpty()) {
        updateStatusMessage(QStringLiteral("Playback requested."));
    }
    return true;
}

bool PlaybackViewModel::executePause()
{
    m_playbackUseCase.pause();
    updateStatusMessage(QStringLiteral("Playback paused."));
    return true;
}

bool PlaybackViewModel::executeStop()
{
    m_playbackUseCase.stop();
    updateStatusMessage(QStringLiteral("Playback stopped."));
    return true;
}

bool PlaybackViewModel::executePrevious()
{
    m_playbackUseCase.previous();
    updateStatusMessage(QStringLiteral("Previous track requested."));
    return true;
}

bool PlaybackViewModel::executeNext()
{
    m_playbackUseCase.next();
    updateStatusMessage(QStringLiteral("Next track requested."));
    return true;
}

bool PlaybackViewModel::executeToggleMute()
{
    m_playbackUseCase.setMuted(!m_muted);
    return true;
}

void PlaybackViewModel::updatePlaybackState(ModelService::PlaybackBackendState state)
{
    const PlaybackState nextState = toPlaybackState(state);
    if (m_playbackState == nextState) {
        return;
    }

    m_playbackState = nextState;
    emit playbackStateChanged();
}

void PlaybackViewModel::updateCurrentTrack(int index, const QString &title, const QString &filePath)
{
    Q_UNUSED(filePath)

    bool changed = false;
    if (m_currentPlaybackIndex != index) {
        m_currentPlaybackIndex = index;
        emit currentPlaybackIndexChanged();
        changed = true;
    }

    if (m_currentPlaybackTitle != title) {
        m_currentPlaybackTitle = title;
        emit currentPlaybackTitleChanged();
        changed = true;
    }

    if (changed && index >= 0 && m_lastPlaybackError.isEmpty()) {
        updateStatusMessage(QStringLiteral("Current track: %1").arg(title));
    }
}

void PlaybackViewModel::updatePosition(qint64 positionMs)
{
    const qint64 normalizedPosition = std::max(positionMs, qint64{0});
    if (m_positionMs == normalizedPosition) {
        return;
    }

    m_positionMs = normalizedPosition;
    emit positionMsChanged();
}

void PlaybackViewModel::updateDuration(qint64 durationMs)
{
    const qint64 normalizedDuration = std::max(durationMs, qint64{0});
    if (m_durationMs == normalizedDuration) {
        return;
    }

    m_durationMs = normalizedDuration;
    emit durationMsChanged();
}

void PlaybackViewModel::updateSeekable(bool seekable)
{
    if (m_seekable == seekable) {
        return;
    }

    m_seekable = seekable;
    emit seekableChanged();
}

void PlaybackViewModel::updateVolume(float volume)
{
    const int nextVolumePercent = toVolumePercent(volume);
    if (m_volumePercent == nextVolumePercent) {
        return;
    }

    m_volumePercent = nextVolumePercent;
    emit volumePercentChanged();
}

void PlaybackViewModel::updateMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }

    m_muted = muted;
    emit mutedChanged();
}

void PlaybackViewModel::updatePlaybackError(const QString &message)
{
    if (m_lastPlaybackError != message) {
        m_lastPlaybackError = message;
        emit lastPlaybackErrorChanged();
    }
    updateStatusMessage(message);
}

void PlaybackViewModel::updateStatusMessage(QString statusMessage)
{
    if (m_statusMessage == statusMessage) {
        return;
    }

    m_statusMessage = std::move(statusMessage);
    emit statusMessageChanged();
}

void PlaybackViewModel::clearPlaybackError()
{
    if (m_lastPlaybackError.isEmpty()) {
        return;
    }

    m_lastPlaybackError.clear();
    emit lastPlaybackErrorChanged();
}

} // namespace AudioPlayer::ViewModel
