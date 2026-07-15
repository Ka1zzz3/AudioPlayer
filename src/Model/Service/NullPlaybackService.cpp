#include "Model/Service/NullPlaybackService.h"

#include <algorithm>
#include <utility>

#include <QtGlobal>

namespace AudioPlayer::Model::Service {

NullPlaybackService::NullPlaybackService(QObject *parent)
    : IPlaybackService(parent)
{
}

QString NullPlaybackService::source() const
{
    return m_source;
}

PlaybackBackendState NullPlaybackService::state() const noexcept
{
    return m_state;
}

qint64 NullPlaybackService::positionMs() const noexcept
{
    return m_positionMs;
}

qint64 NullPlaybackService::durationMs() const noexcept
{
    return m_durationMs;
}

bool NullPlaybackService::seekable() const noexcept
{
    return m_seekable;
}

float NullPlaybackService::volume() const noexcept
{
    return m_volume;
}

bool NullPlaybackService::muted() const noexcept
{
    return m_muted;
}

void NullPlaybackService::setSource(QString filePath)
{
    if (m_source == filePath) {
        return;
    }

    m_source = std::move(filePath);
    m_positionMs = 0;
    m_durationMs = 0;
    m_seekable = false;
    emit sourceChanged(m_source);
    emit positionChanged(m_positionMs);
    emit durationChanged(m_durationMs);
    emit seekableChanged(m_seekable);
}

void NullPlaybackService::play()
{
    if (m_source.isEmpty()) {
        setState(PlaybackBackendState::Stopped);
        emit playbackError(PlaybackBackendError::Resource, QStringLiteral("No audio backend is available yet."));
        return;
    }

    setState(PlaybackBackendState::Playing);
}

void NullPlaybackService::pause()
{
    if (m_state == PlaybackBackendState::Playing) {
        setState(PlaybackBackendState::Paused);
    }
}

void NullPlaybackService::stop()
{
    setState(PlaybackBackendState::Stopped);
    if (m_positionMs != 0) {
        m_positionMs = 0;
        emit positionChanged(m_positionMs);
    }
}

void NullPlaybackService::seekTo(qint64 positionMs)
{
    const qint64 clamped = std::max(positionMs, qint64{0});
    if (!m_seekable || m_positionMs == clamped) {
        return;
    }

    m_positionMs = clamped;
    emit positionChanged(m_positionMs);
}

void NullPlaybackService::setVolume(float volume)
{
    const float clamped = std::clamp(volume, 0.0F, 1.0F);
    if (qFuzzyCompare(m_volume, clamped)) {
        return;
    }

    m_volume = clamped;
    emit volumeChanged(m_volume);
}

void NullPlaybackService::setMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }

    m_muted = muted;
    emit mutedChanged(m_muted);
}

void NullPlaybackService::setState(PlaybackBackendState state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged(m_state);
}

} // namespace AudioPlayer::Model::Service
