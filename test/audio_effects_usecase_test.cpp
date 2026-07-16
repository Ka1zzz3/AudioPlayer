#include "Model/Service/AudioEffectsUseCase.h"

#include <QtTest/QtTest>

using namespace AudioPlayer::Model::Service;

class FakeAudioEffectsService final : public IAudioEffectsService
{
    Q_OBJECT

public:
    explicit FakeAudioEffectsService(QObject *parent = nullptr)
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

class AudioEffectsUseCaseTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultsMirrorServiceSessionState();
    void setPlaybackRateValidatesFixedPresetsAndForwardsToService();
    void unsupportedPlaybackRateIsRejectedBeforeServiceCall();
    void unavailableRateCapabilityShowsRecoverableError();
    void equalizerEnabledBandGainAndPresetForwardToService();
    void invalidBandOrGainAreRejectedBeforeServiceCall();
    void resetEqualizerRestoresDisabledFlatSessionState();
    void serviceSignalsUpdateUseCaseState();
};

namespace {

void compareGains(const EqualizerGainList &actual, const EqualizerGainList &expected)
{
    for (std::size_t i = 0; i < actual.size(); ++i) {
        QCOMPARE(actual[i], expected[i]);
    }
}

} // namespace

void AudioEffectsUseCaseTest::defaultsMirrorServiceSessionState()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);

    QVERIFY(useCase.capabilities().supportsPitchPreservingRate);
    QVERIFY(useCase.capabilities().supportsEqualizer);
    QCOMPARE(useCase.playbackRate(), 1.0);
    QVERIFY(!useCase.equalizerSettings().enabled);
    QVERIFY(useCase.equalizerSettings().preset == EqualizerPreset::Flat);
    QVERIFY(useCase.statusMessage().isEmpty());
    QVERIFY(useCase.lastError().isEmpty());
}

void AudioEffectsUseCaseTest::setPlaybackRateValidatesFixedPresetsAndForwardsToService()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);
    QSignalSpy rateSpy(&useCase, &AudioEffectsUseCase::playbackRateChanged);
    QSignalSpy statusSpy(&useCase, &AudioEffectsUseCase::statusMessageChanged);

    QVERIFY(useCase.setPlaybackRate(1.5));

    QCOMPARE(service.setRateCount(), 1);
    QCOMPARE(service.playbackRate(), 1.5);
    QCOMPARE(useCase.playbackRate(), 1.5);
    QVERIFY(rateSpy.count() >= 1);
    QVERIFY(statusSpy.count() >= 1);
    QVERIFY(useCase.lastError().isEmpty());

    QVERIFY(useCase.resetPlaybackRate());
    QCOMPARE(useCase.playbackRate(), 1.0);
    QCOMPARE(service.setRateCount(), 2);
}

void AudioEffectsUseCaseTest::unsupportedPlaybackRateIsRejectedBeforeServiceCall()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);

    QVERIFY(!useCase.setPlaybackRate(0.8));

    QCOMPARE(service.setRateCount(), 0);
    QCOMPARE(useCase.playbackRate(), 1.0);
    QCOMPARE(useCase.lastError(), QStringLiteral("Unsupported playback rate."));
}

void AudioEffectsUseCaseTest::unavailableRateCapabilityShowsRecoverableError()
{
    FakeAudioEffectsService service;
    service.setCapabilities(unavailableAudioEffectsCapabilities(QStringLiteral("Effects backend unavailable.")));
    AudioEffectsUseCase useCase(&service);

    QVERIFY(!useCase.setPlaybackRate(1.25));

    QCOMPARE(service.setRateCount(), 0);
    QCOMPARE(useCase.lastError(), QStringLiteral("Effects backend unavailable."));
}

void AudioEffectsUseCaseTest::equalizerEnabledBandGainAndPresetForwardToService()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);
    QSignalSpy eqSpy(&useCase, &AudioEffectsUseCase::equalizerSettingsChanged);

    QVERIFY(useCase.setEqualizerEnabled(true));
    QVERIFY(useCase.equalizerSettings().enabled);
    QCOMPARE(service.setEnabledCount(), 1);

    QVERIFY(useCase.setEqualizerBandGain(2, 4.5));
    QCOMPARE(service.setBandGainCount(), 1);
    QCOMPARE(service.lastBandIndex(), 2);
    QCOMPARE(service.lastBandGain(), 4.5);
    QCOMPARE(useCase.equalizerSettings().gainsDb[2], 4.5);
    QVERIFY(useCase.equalizerSettings().preset == EqualizerPreset::Flat);

    QVERIFY(useCase.applyEqualizerPreset(EqualizerPreset::BassBoost));
    QCOMPARE(service.applyPresetCount(), 1);
    QVERIFY(useCase.equalizerSettings().preset == EqualizerPreset::BassBoost);
    compareGains(useCase.equalizerSettings().gainsDb, presetGains(EqualizerPreset::BassBoost));
    QVERIFY(eqSpy.count() >= 3);
}

void AudioEffectsUseCaseTest::invalidBandOrGainAreRejectedBeforeServiceCall()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);

    QVERIFY(!useCase.setEqualizerBandGain(9, 1.0));
    QCOMPARE(service.setBandGainCount(), 0);
    QCOMPARE(useCase.lastError(), QStringLiteral("Invalid equalizer band."));

    QVERIFY(!useCase.setEqualizerBandGain(1, 99.0));
    QCOMPARE(service.setBandGainCount(), 0);
    QCOMPARE(useCase.lastError(), QStringLiteral("Equalizer gain is out of range."));
}

void AudioEffectsUseCaseTest::resetEqualizerRestoresDisabledFlatSessionState()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);

    QVERIFY(useCase.setEqualizerEnabled(true));
    QVERIFY(useCase.applyEqualizerPreset(EqualizerPreset::Treble));
    QVERIFY(useCase.resetEqualizer());

    QCOMPARE(service.resetEqCount(), 1);
    QVERIFY(!useCase.equalizerSettings().enabled);
    QVERIFY(useCase.equalizerSettings().preset == EqualizerPreset::Flat);
    compareGains(useCase.equalizerSettings().gainsDb, flatEqualizerGains());
}

void AudioEffectsUseCaseTest::serviceSignalsUpdateUseCaseState()
{
    FakeAudioEffectsService service;
    AudioEffectsUseCase useCase(&service);
    QSignalSpy capabilitiesSpy(&useCase, &AudioEffectsUseCase::capabilitiesChanged);
    QSignalSpy errorSpy(&useCase, &AudioEffectsUseCase::lastErrorChanged);
    QSignalSpy statusSpy(&useCase, &AudioEffectsUseCase::statusMessageChanged);

    service.setCapabilities(unavailableAudioEffectsCapabilities(QStringLiteral("Missing scaletempo.")));
    QCOMPARE(capabilitiesSpy.count(), 1);
    QVERIFY(!useCase.capabilities().supportsEqualizer);
    QCOMPARE(useCase.capabilities().unavailableReason, QStringLiteral("Missing scaletempo."));

    service.emitStatus(QStringLiteral("Effects backend ready."));
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(useCase.statusMessage(), QStringLiteral("Effects backend ready."));

    service.emitBackendError(QStringLiteral("GStreamer bus error."));
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(useCase.lastError(), QStringLiteral("GStreamer bus error."));
}

QTEST_MAIN(AudioEffectsUseCaseTest)
#include "audio_effects_usecase_test.moc"
