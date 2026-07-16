#pragma once

#include <QString>

#include <array>

namespace AudioPlayer::Model::Service {

inline constexpr int AudioEffectsEqualizerBandCount = 5;
inline constexpr int AudioEffectsPlaybackRateCount = 6;
inline constexpr double AudioEffectsMinimumGainDb = -12.0;
inline constexpr double AudioEffectsMaximumGainDb = 12.0;
inline constexpr double AudioEffectsDefaultPlaybackRate = 1.0;

using PlaybackRateList = std::array<double, AudioEffectsPlaybackRateCount>;
using EqualizerGainList = std::array<double, AudioEffectsEqualizerBandCount>;
using EqualizerFrequencyList = std::array<double, AudioEffectsEqualizerBandCount>;

struct EqualizerBand
{
    int index = -1;
    double centerFrequencyHz = 0.0;
    double gainDb = 0.0;
};

enum class EqualizerPreset
{
    Flat,
    BassBoost,
    Vocal,
    Treble,
};

struct EqualizerSettings
{
    bool enabled = false;
    EqualizerGainList gainsDb = {};
    EqualizerPreset preset = EqualizerPreset::Flat;
};

struct AudioEffectsCapabilities
{
    bool supportsPitchPreservingRate = false;
    bool supportsEqualizer = false;
    PlaybackRateList availableRates = {};
    QString unavailableReason;
};

[[nodiscard]] PlaybackRateList supportedPlaybackRates() noexcept;
[[nodiscard]] EqualizerFrequencyList defaultEqualizerCenterFrequencies() noexcept;
[[nodiscard]] EqualizerGainList flatEqualizerGains() noexcept;
[[nodiscard]] EqualizerGainList presetGains(EqualizerPreset preset) noexcept;
[[nodiscard]] EqualizerSettings defaultEqualizerSettings() noexcept;
[[nodiscard]] AudioEffectsCapabilities unavailableAudioEffectsCapabilities(QString reason = {});
[[nodiscard]] AudioEffectsCapabilities defaultAudioEffectsCapabilities();

[[nodiscard]] bool isSupportedPlaybackRate(double rate) noexcept;
[[nodiscard]] bool isValidEqualizerBandIndex(int index) noexcept;
[[nodiscard]] bool isValidEqualizerGain(double gainDb) noexcept;
[[nodiscard]] double clampedEqualizerGain(double gainDb) noexcept;
[[nodiscard]] EqualizerBand defaultEqualizerBand(int index) noexcept;

[[nodiscard]] QString toString(EqualizerPreset preset);
[[nodiscard]] QString displayLabel(EqualizerPreset preset);
[[nodiscard]] QString displayLabelForPlaybackRate(double rate);

} // namespace AudioPlayer::Model::Service
