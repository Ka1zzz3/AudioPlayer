#pragma once

#include "Model/Service/AudioEffectsTypes.h"

#include <QObject>
#include <QString>

namespace AudioPlayer::Model::Service {

class IAudioEffectsService : public QObject
{
    Q_OBJECT

public:
    explicit IAudioEffectsService(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~IAudioEffectsService() override = default;

    [[nodiscard]] virtual AudioEffectsCapabilities capabilities() const = 0;
    [[nodiscard]] virtual double playbackRate() const noexcept = 0;
    [[nodiscard]] virtual EqualizerSettings equalizerSettings() const = 0;
    [[nodiscard]] virtual QString statusMessage() const = 0;
    [[nodiscard]] virtual QString lastError() const = 0;

public slots:
    virtual void setPlaybackRate(double rate) = 0;
    virtual void setEqualizerEnabled(bool enabled) = 0;
    virtual void setEqualizerBandGain(int bandIndex, double gainDb) = 0;
    virtual void applyEqualizerPreset(AudioPlayer::Model::Service::EqualizerPreset preset) = 0;
    virtual void resetEqualizer() = 0;

signals:
    void capabilitiesChanged();
    void playbackRateChanged(double rate);
    void equalizerSettingsChanged();
    void statusMessageChanged(const QString &message);
    void audioEffectsError(const QString &message);
};

} // namespace AudioPlayer::Model::Service
