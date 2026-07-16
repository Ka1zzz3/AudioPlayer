#include "Model/Service/NullAudioEffectsService.h"

#include <utility>

namespace AudioPlayer::Model::Service {

NullAudioEffectsService::NullAudioEffectsService(QString unavailableReason, QObject *parent)
    : IAudioEffectsService(parent)
    , m_capabilities(unavailableAudioEffectsCapabilities(std::move(unavailableReason)))
    , m_statusMessage(m_capabilities.unavailableReason)
    , m_lastError(m_capabilities.unavailableReason)
{
}

AudioEffectsCapabilities NullAudioEffectsService::capabilities() const
{
    return m_capabilities;
}

double NullAudioEffectsService::playbackRate() const noexcept
{
    return AudioEffectsDefaultPlaybackRate;
}

EqualizerSettings NullAudioEffectsService::equalizerSettings() const
{
    return m_settings;
}

QString NullAudioEffectsService::statusMessage() const
{
    return m_statusMessage;
}

QString NullAudioEffectsService::lastError() const
{
    return m_lastError;
}

void NullAudioEffectsService::setPlaybackRate(double)
{
    reportUnavailable();
}

void NullAudioEffectsService::setEqualizerEnabled(bool)
{
    reportUnavailable();
}

void NullAudioEffectsService::setEqualizerBandGain(int, double)
{
    reportUnavailable();
}

void NullAudioEffectsService::applyEqualizerPreset(EqualizerPreset)
{
    reportUnavailable();
}

void NullAudioEffectsService::resetEqualizer()
{
    reportUnavailable();
}

void NullAudioEffectsService::reportUnavailable()
{
    m_lastError = m_capabilities.unavailableReason.isEmpty()
        ? QStringLiteral("Audio effects are unavailable.")
        : m_capabilities.unavailableReason;
    emit audioEffectsError(m_lastError);
}

} // namespace AudioPlayer::Model::Service
