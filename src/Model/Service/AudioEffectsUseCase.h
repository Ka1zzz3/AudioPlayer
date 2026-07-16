#pragma once

#include "Model/Service/IAudioEffectsService.h"

#include <QObject>
#include <QString>

namespace AudioPlayer::Model::Service {

class AudioEffectsUseCase : public QObject
{
    Q_OBJECT

public:
    explicit AudioEffectsUseCase(IAudioEffectsService *service, QObject *parent = nullptr);

    [[nodiscard]] AudioEffectsCapabilities capabilities() const;
    [[nodiscard]] double playbackRate() const noexcept;
    [[nodiscard]] EqualizerSettings equalizerSettings() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString lastError() const;

    [[nodiscard]] bool setPlaybackRate(double rate);
    [[nodiscard]] bool resetPlaybackRate();
    [[nodiscard]] bool setEqualizerEnabled(bool enabled);
    [[nodiscard]] bool setEqualizerBandGain(int bandIndex, double gainDb);
    [[nodiscard]] bool applyEqualizerPreset(EqualizerPreset preset);
    [[nodiscard]] bool resetEqualizer();

signals:
    void capabilitiesChanged();
    void playbackRateChanged();
    void equalizerSettingsChanged();
    void statusMessageChanged();
    void lastErrorChanged();

private slots:
    void reloadCapabilitiesFromService();
    void handleServicePlaybackRateChanged(double rate);
    void handleServiceEqualizerSettingsChanged();
    void handleServiceStatusMessageChanged(const QString &message);
    void handleServiceError(const QString &message);

private:
    [[nodiscard]] bool ensureServiceAvailableForRate();
    [[nodiscard]] bool ensureServiceAvailableForEqualizer();
    void setStatus(QString message);
    void setError(QString message);
    void clearError();
    void setPlaybackRateLocal(double rate);
    void setEqualizerSettingsLocal(EqualizerSettings settings);

    IAudioEffectsService *m_service = nullptr;
    AudioEffectsCapabilities m_capabilities;
    double m_playbackRate = AudioEffectsDefaultPlaybackRate;
    EqualizerSettings m_equalizerSettings = defaultEqualizerSettings();
    QString m_statusMessage;
    QString m_lastError;
};

} // namespace AudioPlayer::Model::Service
