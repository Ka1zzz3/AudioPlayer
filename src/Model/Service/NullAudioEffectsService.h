#pragma once

#include "Model/Service/IAudioEffectsService.h"

namespace AudioPlayer::Model::Service {

class NullAudioEffectsService final : public IAudioEffectsService
{
    Q_OBJECT

public:
    explicit NullAudioEffectsService(QString unavailableReason, QObject *parent = nullptr);

    [[nodiscard]] AudioEffectsCapabilities capabilities() const override;
    [[nodiscard]] double playbackRate() const noexcept override;
    [[nodiscard]] EqualizerSettings equalizerSettings() const override;
    [[nodiscard]] QString statusMessage() const override;
    [[nodiscard]] QString lastError() const override;

public slots:
    void setPlaybackRate(double rate) override;
    void setEqualizerEnabled(bool enabled) override;
    void setEqualizerBandGain(int bandIndex, double gainDb) override;
    void applyEqualizerPreset(AudioPlayer::Model::Service::EqualizerPreset preset) override;
    void resetEqualizer() override;

private:
    void reportUnavailable();

    AudioEffectsCapabilities m_capabilities;
    EqualizerSettings m_settings = defaultEqualizerSettings();
    QString m_statusMessage;
    QString m_lastError;
};

} // namespace AudioPlayer::Model::Service
