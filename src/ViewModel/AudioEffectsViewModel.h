#pragma once

#include "ViewModel/AudioEffectsViewModelProtocol.h"

#include "Model/Service/AudioEffectsUseCase.h"

#include <memory>

namespace AudioPlayer::ViewModel {

class AudioEffectsViewModel : public AudioEffectsViewModelProtocol
{
    Q_OBJECT
    Q_PROPERTY(bool supportsPitchPreservingRate READ supportsPitchPreservingRate NOTIFY capabilitiesChanged)
    Q_PROPERTY(bool supportsEqualizer READ supportsEqualizer NOTIFY capabilitiesChanged)
    Q_PROPERTY(double playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(QStringList playbackRateLabels READ playbackRateLabels CONSTANT)
    Q_PROPERTY(bool equalizerEnabled READ equalizerEnabled WRITE setEqualizerEnabled NOTIFY equalizerEnabledChanged)
    Q_PROPERTY(double band0GainDb READ band0GainDb NOTIFY equalizerBandGainsChanged)
    Q_PROPERTY(double band1GainDb READ band1GainDb NOTIFY equalizerBandGainsChanged)
    Q_PROPERTY(double band2GainDb READ band2GainDb NOTIFY equalizerBandGainsChanged)
    Q_PROPERTY(double band3GainDb READ band3GainDb NOTIFY equalizerBandGainsChanged)
    Q_PROPERTY(double band4GainDb READ band4GainDb NOTIFY equalizerBandGainsChanged)
    Q_PROPERTY(QString equalizerPreset READ equalizerPreset WRITE setEqualizerPreset NOTIFY equalizerPresetChanged)
    Q_PROPERTY(QStringList equalizerPresetLabels READ equalizerPresetLabels CONSTANT)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *resetPlaybackRateCommand READ resetPlaybackRateCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *resetEqualizerCommand READ resetEqualizerCommand CONSTANT)

public:
    explicit AudioEffectsViewModel(std::shared_ptr<Model::Service::AudioEffectsUseCase> useCase,
                                   QObject *parent = nullptr);

    [[nodiscard]] bool supportsPitchPreservingRate() const noexcept override;
    [[nodiscard]] bool supportsEqualizer() const noexcept override;
    [[nodiscard]] double playbackRate() const noexcept override;
    [[nodiscard]] const QStringList &playbackRateLabels() const noexcept override;
    [[nodiscard]] bool equalizerEnabled() const noexcept override;
    [[nodiscard]] double band0GainDb() const noexcept override;
    [[nodiscard]] double band1GainDb() const noexcept override;
    [[nodiscard]] double band2GainDb() const noexcept override;
    [[nodiscard]] double band3GainDb() const noexcept override;
    [[nodiscard]] double band4GainDb() const noexcept override;
    [[nodiscard]] const QString &equalizerPreset() const noexcept override;
    [[nodiscard]] const QStringList &equalizerPresetLabels() const noexcept override;
    [[nodiscard]] const QString &lastError() const noexcept override;
    [[nodiscard]] const QString &statusMessage() const noexcept override;

    [[nodiscard]] Common::ViewCommand *resetPlaybackRateCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *resetEqualizerCommand() noexcept override;

public slots:
    void setPlaybackRate(double rate) override;
    void setEqualizerEnabled(bool enabled) override;
    void setEqualizerBandGain(int bandIndex, double gainDb) override;
    void setEqualizerPreset(QString presetLabel) override;

private:
    bool executeResetPlaybackRate();
    bool executeResetEqualizer();
    void refreshCapabilities();
    void refreshPlaybackRate();
    void refreshEqualizerSettings();
    void refreshStatusMessage();
    void refreshLastError();
    [[nodiscard]] double bandGain(int index) const noexcept;
    [[nodiscard]] Model::Service::EqualizerPreset presetFromLabel(const QString &label) const noexcept;
    void setLastError(QString error);
    void setStatusMessage(QString status);
    void setEqualizerPresetLabel(QString label);

    std::shared_ptr<Model::Service::AudioEffectsUseCase> m_useCase;
    Common::ViewCommand m_resetPlaybackRateCommand;
    Common::ViewCommand m_resetEqualizerCommand;
    QStringList m_playbackRateLabels;
    QStringList m_equalizerPresetLabels;
    Model::Service::AudioEffectsCapabilities m_capabilities;
    double m_playbackRate = Model::Service::AudioEffectsDefaultPlaybackRate;
    Model::Service::EqualizerSettings m_equalizerSettings = Model::Service::defaultEqualizerSettings();
    QString m_equalizerPreset;
    QString m_lastError;
    QString m_statusMessage;
};

} // namespace AudioPlayer::ViewModel
