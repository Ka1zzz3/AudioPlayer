#include "Model/Service/AudioEffectsTypes.h"

#include <algorithm>
#include <cmath>

namespace AudioPlayer::Model::Service {
namespace {

constexpr double RateTolerance = 0.0001;

} // namespace

PlaybackRateList supportedPlaybackRates() noexcept
{
    return {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
}

EqualizerFrequencyList defaultEqualizerCenterFrequencies() noexcept
{
    return {60.0, 230.0, 910.0, 3600.0, 14000.0};
}

EqualizerGainList flatEqualizerGains() noexcept
{
    return {0.0, 0.0, 0.0, 0.0, 0.0};
}

EqualizerGainList presetGains(EqualizerPreset preset) noexcept
{
    switch (preset) {
    case EqualizerPreset::Flat:
        return flatEqualizerGains();
    case EqualizerPreset::BassBoost:
        return {6.0, 4.0, 1.0, 0.0, 0.0};
    case EqualizerPreset::Vocal:
        return {-2.0, 0.0, 4.0, 3.0, -1.0};
    case EqualizerPreset::Treble:
        return {0.0, 0.0, 1.0, 4.0, 6.0};
    }

    return flatEqualizerGains();
}

EqualizerSettings defaultEqualizerSettings() noexcept
{
    EqualizerSettings settings;
    settings.enabled = false;
    settings.gainsDb = flatEqualizerGains();
    settings.preset = EqualizerPreset::Flat;
    return settings;
}

AudioEffectsCapabilities unavailableAudioEffectsCapabilities(QString reason)
{
    AudioEffectsCapabilities capabilities;
    capabilities.supportsPitchPreservingRate = false;
    capabilities.supportsEqualizer = false;
    capabilities.availableRates = supportedPlaybackRates();
    capabilities.unavailableReason = std::move(reason);
    return capabilities;
}

AudioEffectsCapabilities defaultAudioEffectsCapabilities()
{
    AudioEffectsCapabilities capabilities;
    capabilities.supportsPitchPreservingRate = true;
    capabilities.supportsEqualizer = true;
    capabilities.availableRates = supportedPlaybackRates();
    return capabilities;
}

bool isSupportedPlaybackRate(double rate) noexcept
{
    const PlaybackRateList rates = supportedPlaybackRates();
    return std::any_of(rates.cbegin(), rates.cend(), [rate](double supportedRate) {
        return std::abs(supportedRate - rate) <= RateTolerance;
    });
}

bool isValidEqualizerBandIndex(int index) noexcept
{
    return index >= 0 && index < AudioEffectsEqualizerBandCount;
}

bool isValidEqualizerGain(double gainDb) noexcept
{
    return gainDb >= AudioEffectsMinimumGainDb && gainDb <= AudioEffectsMaximumGainDb;
}

double clampedEqualizerGain(double gainDb) noexcept
{
    return std::clamp(gainDb, AudioEffectsMinimumGainDb, AudioEffectsMaximumGainDb);
}

EqualizerBand defaultEqualizerBand(int index) noexcept
{
    const EqualizerFrequencyList frequencies = defaultEqualizerCenterFrequencies();
    if (!isValidEqualizerBandIndex(index)) {
        return {};
    }

    return EqualizerBand{index, frequencies[static_cast<std::size_t>(index)], 0.0};
}

QString toString(EqualizerPreset preset)
{
    switch (preset) {
    case EqualizerPreset::Flat:
        return QStringLiteral("flat");
    case EqualizerPreset::BassBoost:
        return QStringLiteral("bass_boost");
    case EqualizerPreset::Vocal:
        return QStringLiteral("vocal");
    case EqualizerPreset::Treble:
        return QStringLiteral("treble");
    }

    return QStringLiteral("unknown");
}

QString displayLabel(EqualizerPreset preset)
{
    switch (preset) {
    case EqualizerPreset::Flat:
        return QStringLiteral("Flat");
    case EqualizerPreset::BassBoost:
        return QStringLiteral("Bass Boost");
    case EqualizerPreset::Vocal:
        return QStringLiteral("Vocal");
    case EqualizerPreset::Treble:
        return QStringLiteral("Treble");
    }

    return QStringLiteral("Unknown");
}

QString displayLabelForPlaybackRate(double rate)
{
    return QStringLiteral("%1x").arg(rate, 0, 'g', 3);
}

} // namespace AudioPlayer::Model::Service
