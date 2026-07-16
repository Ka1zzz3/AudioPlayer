#pragma once

#include "Common/ViewCommand.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace AudioPlayer::ViewModel {

class AudioEffectsViewModelProtocol : public QObject
{
    Q_OBJECT

public:
    explicit AudioEffectsViewModelProtocol(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] virtual bool supportsPitchPreservingRate() const noexcept = 0;
    [[nodiscard]] virtual bool supportsEqualizer() const noexcept = 0;
    [[nodiscard]] virtual double playbackRate() const noexcept = 0;
    [[nodiscard]] virtual const QStringList &playbackRateLabels() const noexcept = 0;
    [[nodiscard]] virtual bool equalizerEnabled() const noexcept = 0;
    [[nodiscard]] virtual double band0GainDb() const noexcept = 0;
    [[nodiscard]] virtual double band1GainDb() const noexcept = 0;
    [[nodiscard]] virtual double band2GainDb() const noexcept = 0;
    [[nodiscard]] virtual double band3GainDb() const noexcept = 0;
    [[nodiscard]] virtual double band4GainDb() const noexcept = 0;
    [[nodiscard]] virtual const QString &equalizerPreset() const noexcept = 0;
    [[nodiscard]] virtual const QStringList &equalizerPresetLabels() const noexcept = 0;
    [[nodiscard]] virtual const QString &lastError() const noexcept = 0;
    [[nodiscard]] virtual const QString &statusMessage() const noexcept = 0;

    [[nodiscard]] virtual Common::ViewCommand *resetPlaybackRateCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *resetEqualizerCommand() noexcept = 0;

public slots:
    virtual void setPlaybackRate(double rate) = 0;
    virtual void setEqualizerEnabled(bool enabled) = 0;
    virtual void setEqualizerBandGain(int bandIndex, double gainDb) = 0;
    virtual void setEqualizerPreset(QString presetLabel) = 0;

signals:
    void capabilitiesChanged();
    void playbackRateChanged();
    void equalizerEnabledChanged();
    void equalizerBandGainsChanged();
    void equalizerPresetChanged();
    void lastErrorChanged();
    void statusMessageChanged();
};

} // namespace AudioPlayer::ViewModel
