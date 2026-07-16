#include "Model/Service/AudioEffectsUseCase.h"

#include <utility>

namespace AudioPlayer::Model::Service {

AudioEffectsUseCase::AudioEffectsUseCase(IAudioEffectsService *service, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_capabilities(service != nullptr ? service->capabilities() : unavailableAudioEffectsCapabilities(QStringLiteral("Audio effects service is unavailable.")))
    , m_playbackRate(service != nullptr ? service->playbackRate() : AudioEffectsDefaultPlaybackRate)
    , m_equalizerSettings(service != nullptr ? service->equalizerSettings() : defaultEqualizerSettings())
    , m_statusMessage(service != nullptr ? service->statusMessage() : QStringLiteral("Audio effects service is unavailable."))
    , m_lastError(service != nullptr ? service->lastError() : QStringLiteral("Audio effects service is unavailable."))
{
    if (m_service == nullptr) {
        return;
    }

    connect(m_service, &IAudioEffectsService::capabilitiesChanged, this, &AudioEffectsUseCase::reloadCapabilitiesFromService);
    connect(m_service, &IAudioEffectsService::playbackRateChanged, this, &AudioEffectsUseCase::handleServicePlaybackRateChanged);
    connect(m_service, &IAudioEffectsService::equalizerSettingsChanged, this, &AudioEffectsUseCase::handleServiceEqualizerSettingsChanged);
    connect(m_service, &IAudioEffectsService::statusMessageChanged, this, &AudioEffectsUseCase::handleServiceStatusMessageChanged);
    connect(m_service, &IAudioEffectsService::audioEffectsError, this, &AudioEffectsUseCase::handleServiceError);
}

AudioEffectsCapabilities AudioEffectsUseCase::capabilities() const
{
    return m_capabilities;
}

double AudioEffectsUseCase::playbackRate() const noexcept
{
    return m_playbackRate;
}

EqualizerSettings AudioEffectsUseCase::equalizerSettings() const
{
    return m_equalizerSettings;
}

QString AudioEffectsUseCase::statusMessage() const
{
    return m_statusMessage;
}

QString AudioEffectsUseCase::lastError() const
{
    return m_lastError;
}

bool AudioEffectsUseCase::setPlaybackRate(double rate)
{
    if (!ensureServiceAvailableForRate()) {
        return false;
    }
    if (!isSupportedPlaybackRate(rate)) {
        setError(QStringLiteral("Unsupported playback rate."));
        return false;
    }

    m_service->setPlaybackRate(rate);
    setPlaybackRateLocal(rate);
    setStatus(QStringLiteral("Playback speed set to %1.").arg(displayLabelForPlaybackRate(rate)));
    clearError();
    return true;
}

bool AudioEffectsUseCase::resetPlaybackRate()
{
    return setPlaybackRate(AudioEffectsDefaultPlaybackRate);
}

bool AudioEffectsUseCase::setEqualizerEnabled(bool enabled)
{
    if (!ensureServiceAvailableForEqualizer()) {
        return false;
    }

    m_service->setEqualizerEnabled(enabled);
    EqualizerSettings settings = m_equalizerSettings;
    settings.enabled = enabled;
    setEqualizerSettingsLocal(settings);
    setStatus(enabled ? QStringLiteral("Equalizer enabled.") : QStringLiteral("Equalizer disabled."));
    clearError();
    return true;
}

bool AudioEffectsUseCase::setEqualizerBandGain(int bandIndex, double gainDb)
{
    if (!ensureServiceAvailableForEqualizer()) {
        return false;
    }
    if (!isValidEqualizerBandIndex(bandIndex)) {
        setError(QStringLiteral("Invalid equalizer band."));
        return false;
    }
    if (!isValidEqualizerGain(gainDb)) {
        setError(QStringLiteral("Equalizer gain is out of range."));
        return false;
    }

    m_service->setEqualizerBandGain(bandIndex, gainDb);
    EqualizerSettings settings = m_equalizerSettings;
    settings.gainsDb[static_cast<std::size_t>(bandIndex)] = gainDb;
    settings.preset = EqualizerPreset::Flat;
    setEqualizerSettingsLocal(settings);
    setStatus(QStringLiteral("Equalizer band updated."));
    clearError();
    return true;
}

bool AudioEffectsUseCase::applyEqualizerPreset(EqualizerPreset preset)
{
    if (!ensureServiceAvailableForEqualizer()) {
        return false;
    }

    m_service->applyEqualizerPreset(preset);
    EqualizerSettings settings = m_equalizerSettings;
    settings.gainsDb = presetGains(preset);
    settings.preset = preset;
    setEqualizerSettingsLocal(settings);
    setStatus(QStringLiteral("Equalizer preset set to %1.").arg(displayLabel(preset)));
    clearError();
    return true;
}

bool AudioEffectsUseCase::resetEqualizer()
{
    if (!ensureServiceAvailableForEqualizer()) {
        return false;
    }

    m_service->resetEqualizer();
    EqualizerSettings settings = m_equalizerSettings;
    settings.enabled = false;
    settings.gainsDb = flatEqualizerGains();
    settings.preset = EqualizerPreset::Flat;
    setEqualizerSettingsLocal(settings);
    setStatus(QStringLiteral("Equalizer reset to Flat."));
    clearError();
    return true;
}

void AudioEffectsUseCase::reloadCapabilitiesFromService()
{
    if (m_service == nullptr) {
        return;
    }

    m_capabilities = m_service->capabilities();
    emit capabilitiesChanged();
}

void AudioEffectsUseCase::handleServicePlaybackRateChanged(double rate)
{
    setPlaybackRateLocal(rate);
}

void AudioEffectsUseCase::handleServiceEqualizerSettingsChanged()
{
    if (m_service == nullptr) {
        return;
    }

    setEqualizerSettingsLocal(m_service->equalizerSettings());
}

void AudioEffectsUseCase::handleServiceStatusMessageChanged(const QString &message)
{
    setStatus(message);
}

void AudioEffectsUseCase::handleServiceError(const QString &message)
{
    setError(message);
}

bool AudioEffectsUseCase::ensureServiceAvailableForRate()
{
    if (m_service == nullptr) {
        setError(QStringLiteral("Audio effects service is unavailable."));
        return false;
    }
    if (!m_capabilities.supportsPitchPreservingRate) {
        setError(m_capabilities.unavailableReason.isEmpty() ? QStringLiteral("Pitch-preserving playback speed is unavailable.") : m_capabilities.unavailableReason);
        return false;
    }
    return true;
}

bool AudioEffectsUseCase::ensureServiceAvailableForEqualizer()
{
    if (m_service == nullptr) {
        setError(QStringLiteral("Audio effects service is unavailable."));
        return false;
    }
    if (!m_capabilities.supportsEqualizer) {
        setError(m_capabilities.unavailableReason.isEmpty() ? QStringLiteral("Equalizer is unavailable.") : m_capabilities.unavailableReason);
        return false;
    }
    return true;
}

void AudioEffectsUseCase::setStatus(QString message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = std::move(message);
    emit statusMessageChanged();
}

void AudioEffectsUseCase::setError(QString message)
{
    if (m_lastError == message) {
        return;
    }

    m_lastError = std::move(message);
    emit lastErrorChanged();
}

void AudioEffectsUseCase::clearError()
{
    setError({});
}

void AudioEffectsUseCase::setPlaybackRateLocal(double rate)
{
    if (m_playbackRate == rate) {
        return;
    }

    m_playbackRate = rate;
    emit playbackRateChanged();
}

void AudioEffectsUseCase::setEqualizerSettingsLocal(EqualizerSettings settings)
{
    if (m_equalizerSettings.enabled == settings.enabled
        && m_equalizerSettings.gainsDb == settings.gainsDb
        && m_equalizerSettings.preset == settings.preset) {
        return;
    }

    m_equalizerSettings = settings;
    emit equalizerSettingsChanged();
}

} // namespace AudioPlayer::Model::Service
