#include "Model/Service/AudioEffectsUseCase.h"
#include "ViewModel/AudioEffectsViewModel.h"

#include <QtTest/QtTest>

using namespace AudioPlayer::Model::Service;
using AudioPlayer::ViewModel::AudioEffectsViewModel;

class AudioEffectsViewModelFakeService final : public IAudioEffectsService
{
    Q_OBJECT

public:
    explicit AudioEffectsViewModelFakeService(QObject *parent = nullptr)
        : IAudioEffectsService(parent)
    {
    }

    [[nodiscard]] AudioEffectsCapabilities capabilities() const override { return m_capabilities; }
    [[nodiscard]] double playbackRate() const noexcept override { return m_playbackRate; }
    [[nodiscard]] EqualizerSettings equalizerSettings() const override { return m_settings; }
    [[nodiscard]] QString statusMessage() const override { return m_statusMessage; }
    [[nodiscard]] QString lastError() const override { return m_lastError; }

    [[nodiscard]] int setRateCount() const noexcept { return m_setRateCount; }
    [[nodiscard]] int setEnabledCount() const noexcept { return m_setEnabledCount; }
    [[nodiscard]] int setBandGainCount() const noexcept { return m_setBandGainCount; }
    [[nodiscard]] int applyPresetCount() const noexcept { return m_applyPresetCount; }
    [[nodiscard]] int resetEqCount() const noexcept { return m_resetEqCount; }
    [[nodiscard]] int lastBandIndex() const noexcept { return m_lastBandIndex; }
    [[nodiscard]] double lastBandGain() const noexcept { return m_lastBandGain; }

    void setCapabilities(AudioEffectsCapabilities capabilities)
    {
        m_capabilities = std::move(capabilities);
        emit capabilitiesChanged();
    }

public slots:
    void setPlaybackRate(double rate) override
    {
        ++m_setRateCount;
        m_playbackRate = rate;
        emit playbackRateChanged(rate);
    }

    void setEqualizerEnabled(bool enabled) override
    {
        ++m_setEnabledCount;
        m_settings.enabled = enabled;
        emit equalizerSettingsChanged();
    }

    void setEqualizerBandGain(int bandIndex, double gainDb) override
    {
        ++m_setBandGainCount;
        m_lastBandIndex = bandIndex;
        m_lastBandGain = gainDb;
        m_settings.gainsDb[static_cast<std::size_t>(bandIndex)] = gainDb;
        m_settings.preset = EqualizerPreset::Flat;
        emit equalizerSettingsChanged();
    }

    void applyEqualizerPreset(EqualizerPreset preset) override
    {
        ++m_applyPresetCount;
        m_settings.preset = preset;
        m_settings.gainsDb = presetGains(preset);
        emit equalizerSettingsChanged();
    }

    void resetEqualizer() override
    {
        ++m_resetEqCount;
        m_settings = defaultEqualizerSettings();
        emit equalizerSettingsChanged();
    }

    void emitBackendError(QString message)
    {
        m_lastError = std::move(message);
        emit audioEffectsError(m_lastError);
    }

    void emitStatus(QString message)
    {
        m_statusMessage = std::move(message);
        emit statusMessageChanged(m_statusMessage);
    }

private:
    AudioEffectsCapabilities m_capabilities = defaultAudioEffectsCapabilities();
    double m_playbackRate = AudioEffectsDefaultPlaybackRate;
    EqualizerSettings m_settings = defaultEqualizerSettings();
    QString m_statusMessage;
    QString m_lastError;
    int m_setRateCount = 0;
    int m_setEnabledCount = 0;
    int m_setBandGainCount = 0;
    int m_applyPresetCount = 0;
    int m_resetEqCount = 0;
    int m_lastBandIndex = -1;
    double m_lastBandGain = 0.0;
};

class AudioEffectsViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsExposeCapabilitiesLabelsAndCommands();
    void playbackRateChangesPropagateThroughUseCase();
    void invalidPlaybackRateSurfacesRecoverableError();
    void equalizerControlsPropagateSettingsAndPresetLabels();
    void resetCommandsRestoreSessionDefaults();
    void backendSignalsUpdateStatusErrorAndCapabilities();
    void unavailableCapabilitiesDisableOperationsWithError();
};

namespace {

std::shared_ptr<AudioEffectsUseCase> makeUseCase(AudioEffectsViewModelFakeService &service)
{
    return std::make_shared<AudioEffectsUseCase>(&service);
}

} // namespace

void AudioEffectsViewModelTest::defaultsExposeCapabilitiesLabelsAndCommands()
{
    AudioEffectsViewModelFakeService service;
    AudioEffectsViewModel viewModel(makeUseCase(service));

    QVERIFY(viewModel.supportsPitchPreservingRate());
    QVERIFY(viewModel.supportsEqualizer());
    QCOMPARE(viewModel.playbackRate(), 1.0);
    QCOMPARE(viewModel.playbackRateLabels(), QStringList({QStringLiteral("0.5x"),
                                                          QStringLiteral("0.75x"),
                                                          QStringLiteral("1x"),
                                                          QStringLiteral("1.25x"),
                                                          QStringLiteral("1.5x"),
                                                          QStringLiteral("2x")}));
    QVERIFY(!viewModel.equalizerEnabled());
    QCOMPARE(viewModel.band0GainDb(), 0.0);
    QCOMPARE(viewModel.band1GainDb(), 0.0);
    QCOMPARE(viewModel.band2GainDb(), 0.0);
    QCOMPARE(viewModel.band3GainDb(), 0.0);
    QCOMPARE(viewModel.band4GainDb(), 0.0);
    QCOMPARE(viewModel.equalizerPreset(), QStringLiteral("Flat"));
    QCOMPARE(viewModel.equalizerPresetLabels(), QStringList({QStringLiteral("Flat"),
                                                             QStringLiteral("Bass Boost"),
                                                             QStringLiteral("Vocal"),
                                                             QStringLiteral("Treble")}));
    QVERIFY(viewModel.lastError().isEmpty());
    QVERIFY(viewModel.statusMessage().isEmpty());
    QVERIFY(viewModel.resetPlaybackRateCommand() != nullptr);
    QVERIFY(viewModel.resetEqualizerCommand() != nullptr);
}

void AudioEffectsViewModelTest::playbackRateChangesPropagateThroughUseCase()
{
    AudioEffectsViewModelFakeService service;
    AudioEffectsViewModel viewModel(makeUseCase(service));
    QSignalSpy rateSpy(&viewModel, &AudioEffectsViewModel::playbackRateChanged);
    QSignalSpy statusSpy(&viewModel, &AudioEffectsViewModel::statusMessageChanged);

    viewModel.setPlaybackRate(1.5);

    QCOMPARE(service.setRateCount(), 1);
    QCOMPARE(service.playbackRate(), 1.5);
    QCOMPARE(viewModel.playbackRate(), 1.5);
    QVERIFY(rateSpy.count() >= 1);
    QVERIFY(statusSpy.count() >= 1);
    QVERIFY(viewModel.statusMessage().contains(QStringLiteral("1.5x")));
    QVERIFY(viewModel.lastError().isEmpty());
}

void AudioEffectsViewModelTest::invalidPlaybackRateSurfacesRecoverableError()
{
    AudioEffectsViewModelFakeService service;
    AudioEffectsViewModel viewModel(makeUseCase(service));
    QSignalSpy errorSpy(&viewModel, &AudioEffectsViewModel::lastErrorChanged);

    viewModel.setPlaybackRate(0.8);

    QCOMPARE(service.setRateCount(), 0);
    QCOMPARE(viewModel.playbackRate(), 1.0);
    QCOMPARE(viewModel.lastError(), QStringLiteral("Unsupported playback rate."));
    QCOMPARE(errorSpy.count(), 1);
}

void AudioEffectsViewModelTest::equalizerControlsPropagateSettingsAndPresetLabels()
{
    AudioEffectsViewModelFakeService service;
    AudioEffectsViewModel viewModel(makeUseCase(service));
    QSignalSpy enabledSpy(&viewModel, &AudioEffectsViewModel::equalizerEnabledChanged);
    QSignalSpy gainsSpy(&viewModel, &AudioEffectsViewModel::equalizerBandGainsChanged);
    QSignalSpy presetSpy(&viewModel, &AudioEffectsViewModel::equalizerPresetChanged);

    viewModel.setEqualizerEnabled(true);
    QVERIFY(viewModel.equalizerEnabled());
    QCOMPARE(service.setEnabledCount(), 1);
    QVERIFY(enabledSpy.count() >= 1);

    viewModel.setEqualizerBandGain(2, 4.5);
    QCOMPARE(service.setBandGainCount(), 1);
    QCOMPARE(service.lastBandIndex(), 2);
    QCOMPARE(service.lastBandGain(), 4.5);
    QCOMPARE(viewModel.band2GainDb(), 4.5);
    QCOMPARE(viewModel.equalizerPreset(), QStringLiteral("Flat"));
    QVERIFY(gainsSpy.count() >= 1);

    viewModel.setEqualizerPreset(QStringLiteral("Bass Boost"));
    QCOMPARE(service.applyPresetCount(), 1);
    QCOMPARE(viewModel.equalizerPreset(), QStringLiteral("Bass Boost"));
    QCOMPARE(viewModel.band0GainDb(), 6.0);
    QCOMPARE(viewModel.band1GainDb(), 4.0);
    QCOMPARE(viewModel.band2GainDb(), 1.0);
    QCOMPARE(viewModel.band3GainDb(), 0.0);
    QCOMPARE(viewModel.band4GainDb(), 0.0);
    QVERIFY(presetSpy.count() >= 1);
}

void AudioEffectsViewModelTest::resetCommandsRestoreSessionDefaults()
{
    AudioEffectsViewModelFakeService service;
    AudioEffectsViewModel viewModel(makeUseCase(service));

    viewModel.setPlaybackRate(2.0);
    viewModel.setEqualizerEnabled(true);
    viewModel.setEqualizerPreset(QStringLiteral("Treble"));

    QVERIFY(viewModel.resetPlaybackRateCommand()->execute());
    QCOMPARE(viewModel.playbackRate(), 1.0);
    QCOMPARE(service.setRateCount(), 2);

    QVERIFY(viewModel.resetEqualizerCommand()->execute());
    QVERIFY(!viewModel.equalizerEnabled());
    QCOMPARE(viewModel.equalizerPreset(), QStringLiteral("Flat"));
    QCOMPARE(viewModel.band4GainDb(), 0.0);
    QCOMPARE(service.resetEqCount(), 1);
}

void AudioEffectsViewModelTest::backendSignalsUpdateStatusErrorAndCapabilities()
{
    AudioEffectsViewModelFakeService service;
    AudioEffectsViewModel viewModel(makeUseCase(service));
    QSignalSpy statusSpy(&viewModel, &AudioEffectsViewModel::statusMessageChanged);
    QSignalSpy errorSpy(&viewModel, &AudioEffectsViewModel::lastErrorChanged);
    QSignalSpy capabilitiesSpy(&viewModel, &AudioEffectsViewModel::capabilitiesChanged);

    service.emitStatus(QStringLiteral("Effects backend ready."));
    QCOMPARE(viewModel.statusMessage(), QStringLiteral("Effects backend ready."));
    QCOMPARE(statusSpy.count(), 1);

    service.emitBackendError(QStringLiteral("Effects backend failed."));
    QCOMPARE(viewModel.lastError(), QStringLiteral("Effects backend failed."));
    QCOMPARE(errorSpy.count(), 1);

    service.setCapabilities(unavailableAudioEffectsCapabilities(QStringLiteral("GStreamer effects unavailable.")));
    QVERIFY(!viewModel.supportsPitchPreservingRate());
    QVERIFY(!viewModel.supportsEqualizer());
    QCOMPARE(capabilitiesSpy.count(), 1);
}

void AudioEffectsViewModelTest::unavailableCapabilitiesDisableOperationsWithError()
{
    AudioEffectsViewModelFakeService service;
    service.setCapabilities(unavailableAudioEffectsCapabilities(QStringLiteral("Effects backend unavailable.")));
    AudioEffectsViewModel viewModel(makeUseCase(service));

    viewModel.setPlaybackRate(1.25);
    QCOMPARE(service.setRateCount(), 0);
    QCOMPARE(viewModel.lastError(), QStringLiteral("Effects backend unavailable."));

    viewModel.setEqualizerEnabled(true);
    QCOMPARE(service.setEnabledCount(), 0);
    QCOMPARE(viewModel.lastError(), QStringLiteral("Effects backend unavailable."));
}

QTEST_MAIN(AudioEffectsViewModelTest)
#include "audio_effects_viewmodel_test.moc"
