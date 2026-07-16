#include "ViewModel/AudioEffectsViewModel.h"

#include <QtGlobal>

#include <utility>

namespace AudioPlayer::ViewModel {

using AudioPlayer::Model::Service::AudioEffectsDefaultPlaybackRate;
using AudioPlayer::Model::Service::EqualizerPreset;
using AudioPlayer::Model::Service::displayLabel;
using AudioPlayer::Model::Service::displayLabelForPlaybackRate;
using AudioPlayer::Model::Service::supportedPlaybackRates;

AudioEffectsViewModel::AudioEffectsViewModel(std::shared_ptr<Model::Service::AudioEffectsUseCase> useCase, QObject *parent)
    : AudioEffectsViewModelProtocol(parent)
    , m_useCase(std::move(useCase))
    , m_resetPlaybackRateCommand(QStringLiteral("resetPlaybackRate"), [this] { return executeResetPlaybackRate(); }, this)
    , m_resetEqualizerCommand(QStringLiteral("resetEqualizer"), [this] { return executeResetEqualizer(); }, this)
    , m_equalizerPreset(displayLabel(EqualizerPreset::Flat))
{
    Q_ASSERT(m_useCase != nullptr);

    for (const double rate : supportedPlaybackRates()) {
        m_playbackRateLabels.push_back(displayLabelForPlaybackRate(rate));
    }
    m_equalizerPresetLabels = {displayLabel(EqualizerPreset::Flat),
                               displayLabel(EqualizerPreset::BassBoost),
                               displayLabel(EqualizerPreset::Vocal),
                               displayLabel(EqualizerPreset::Treble)};

    connect(m_useCase.get(), &Model::Service::AudioEffectsUseCase::capabilitiesChanged, this, &AudioEffectsViewModel::refreshCapabilities);
    connect(m_useCase.get(), &Model::Service::AudioEffectsUseCase::playbackRateChanged, this, &AudioEffectsViewModel::refreshPlaybackRate);
    connect(m_useCase.get(), &Model::Service::AudioEffectsUseCase::equalizerSettingsChanged, this, &AudioEffectsViewModel::refreshEqualizerSettings);
    connect(m_useCase.get(), &Model::Service::AudioEffectsUseCase::statusMessageChanged, this, &AudioEffectsViewModel::refreshStatusMessage);
    connect(m_useCase.get(), &Model::Service::AudioEffectsUseCase::lastErrorChanged, this, &AudioEffectsViewModel::refreshLastError);

    refreshCapabilities();
    refreshPlaybackRate();
    refreshEqualizerSettings();
    refreshStatusMessage();
    refreshLastError();
}

bool AudioEffectsViewModel::supportsPitchPreservingRate() const noexcept
{
    return m_capabilities.supportsPitchPreservingRate;
}

bool AudioEffectsViewModel::supportsEqualizer() const noexcept
{
    return m_capabilities.supportsEqualizer;
}

double AudioEffectsViewModel::playbackRate() const noexcept
{
    return m_playbackRate;
}

const QStringList &AudioEffectsViewModel::playbackRateLabels() const noexcept
{
    return m_playbackRateLabels;
}

bool AudioEffectsViewModel::equalizerEnabled() const noexcept
{
    return m_equalizerSettings.enabled;
}

double AudioEffectsViewModel::band0GainDb() const noexcept
{
    return bandGain(0);
}

double AudioEffectsViewModel::band1GainDb() const noexcept
{
    return bandGain(1);
}

double AudioEffectsViewModel::band2GainDb() const noexcept
{
    return bandGain(2);
}

double AudioEffectsViewModel::band3GainDb() const noexcept
{
    return bandGain(3);
}

double AudioEffectsViewModel::band4GainDb() const noexcept
{
    return bandGain(4);
}

const QString &AudioEffectsViewModel::equalizerPreset() const noexcept
{
    return m_equalizerPreset;
}

const QStringList &AudioEffectsViewModel::equalizerPresetLabels() const noexcept
{
    return m_equalizerPresetLabels;
}

const QString &AudioEffectsViewModel::lastError() const noexcept
{
    return m_lastError;
}

const QString &AudioEffectsViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
}

Common::ViewCommand *AudioEffectsViewModel::resetPlaybackRateCommand() noexcept
{
    return &m_resetPlaybackRateCommand;
}

Common::ViewCommand *AudioEffectsViewModel::resetEqualizerCommand() noexcept
{
    return &m_resetEqualizerCommand;
}

void AudioEffectsViewModel::setPlaybackRate(double rate)
{
    if (!m_useCase->setPlaybackRate(rate)) {
        refreshLastError();
        return;
    }
    refreshPlaybackRate();
    refreshStatusMessage();
    refreshLastError();
}

void AudioEffectsViewModel::setEqualizerEnabled(bool enabled)
{
    if (!m_useCase->setEqualizerEnabled(enabled)) {
        refreshLastError();
        return;
    }
    refreshEqualizerSettings();
    refreshStatusMessage();
    refreshLastError();
}

void AudioEffectsViewModel::setEqualizerBandGain(int bandIndex, double gainDb)
{
    if (!m_useCase->setEqualizerBandGain(bandIndex, gainDb)) {
        refreshLastError();
        return;
    }
    refreshEqualizerSettings();
    refreshStatusMessage();
    refreshLastError();
}

void AudioEffectsViewModel::setEqualizerPreset(QString presetLabel)
{
    if (!m_useCase->applyEqualizerPreset(presetFromLabel(presetLabel))) {
        refreshLastError();
        return;
    }
    refreshEqualizerSettings();
    refreshStatusMessage();
    refreshLastError();
}

bool AudioEffectsViewModel::executeResetPlaybackRate()
{
    const bool ok = m_useCase->resetPlaybackRate();
    refreshPlaybackRate();
    refreshStatusMessage();
    refreshLastError();
    return ok;
}

bool AudioEffectsViewModel::executeResetEqualizer()
{
    const bool ok = m_useCase->resetEqualizer();
    refreshEqualizerSettings();
    refreshStatusMessage();
    refreshLastError();
    return ok;
}

void AudioEffectsViewModel::refreshCapabilities()
{
    const auto capabilities = m_useCase->capabilities();
    if (m_capabilities.supportsPitchPreservingRate == capabilities.supportsPitchPreservingRate
        && m_capabilities.supportsEqualizer == capabilities.supportsEqualizer
        && m_capabilities.availableRates == capabilities.availableRates
        && m_capabilities.unavailableReason == capabilities.unavailableReason) {
        return;
    }

    m_capabilities = capabilities;
    emit capabilitiesChanged();
}

void AudioEffectsViewModel::refreshPlaybackRate()
{
    const double rate = m_useCase->playbackRate();
    if (m_playbackRate == rate) {
        return;
    }

    m_playbackRate = rate;
    emit playbackRateChanged();
}

void AudioEffectsViewModel::refreshEqualizerSettings()
{
    const auto settings = m_useCase->equalizerSettings();
    const bool enabledChanged = m_equalizerSettings.enabled != settings.enabled;
    const bool gainsChanged = m_equalizerSettings.gainsDb != settings.gainsDb;
    const bool presetChanged = m_equalizerSettings.preset != settings.preset;
    if (!enabledChanged && !gainsChanged && !presetChanged) {
        return;
    }

    m_equalizerSettings = settings;
    setEqualizerPresetLabel(displayLabel(settings.preset));
    if (enabledChanged) {
        emit equalizerEnabledChanged();
    }
    if (gainsChanged) {
        emit equalizerBandGainsChanged();
    }
    if (presetChanged) {
        emit equalizerPresetChanged();
    }
}

void AudioEffectsViewModel::refreshStatusMessage()
{
    setStatusMessage(m_useCase->statusMessage());
}

void AudioEffectsViewModel::refreshLastError()
{
    setLastError(m_useCase->lastError());
}

double AudioEffectsViewModel::bandGain(int index) const noexcept
{
    return m_equalizerSettings.gainsDb[static_cast<std::size_t>(index)];
}

EqualizerPreset AudioEffectsViewModel::presetFromLabel(const QString &label) const noexcept
{
    const QString normalized = label.trimmed().toLower();
    if (normalized == QStringLiteral("bass boost") || normalized == QStringLiteral("bass_boost")) {
        return EqualizerPreset::BassBoost;
    }
    if (normalized == QStringLiteral("vocal")) {
        return EqualizerPreset::Vocal;
    }
    if (normalized == QStringLiteral("treble")) {
        return EqualizerPreset::Treble;
    }
    return EqualizerPreset::Flat;
}

void AudioEffectsViewModel::setLastError(QString error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = std::move(error);
    emit lastErrorChanged();
}

void AudioEffectsViewModel::setStatusMessage(QString status)
{
    if (m_statusMessage == status) {
        return;
    }
    m_statusMessage = std::move(status);
    emit statusMessageChanged();
}

void AudioEffectsViewModel::setEqualizerPresetLabel(QString label)
{
    m_equalizerPreset = std::move(label);
}

} // namespace AudioPlayer::ViewModel
