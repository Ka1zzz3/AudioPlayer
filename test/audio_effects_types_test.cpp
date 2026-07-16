#include "Model/Service/AudioEffectsTypes.h"

#include <QtTest/QtTest>

using namespace AudioPlayer::Model::Service;

class AudioEffectsTypesTest : public QObject
{
    Q_OBJECT

private slots:
    void fixedPlaybackRatesAreStableAndValidated();
    void equalizerBandsUsePlannedFiveBandFrequencies();
    void equalizerGainRangeIsStableAndClampable();
    void equalizerPresetsHaveStableGainMaps();
    void defaultSettingsAreSessionDefaults();
    void capabilitiesRepresentSupportedAndUnavailableBackends();
    void labelsAreStableForViewModels();
};

namespace {

void compareGains(const EqualizerGainList &actual, const EqualizerGainList &expected)
{
    for (std::size_t i = 0; i < actual.size(); ++i) {
        QCOMPARE(actual[i], expected[i]);
    }
}

} // namespace

void AudioEffectsTypesTest::fixedPlaybackRatesAreStableAndValidated()
{
    const PlaybackRateList rates = supportedPlaybackRates();

    QCOMPARE(rates.size(), std::size_t{6});
    QCOMPARE(rates[0], 0.5);
    QCOMPARE(rates[1], 0.75);
    QCOMPARE(rates[2], 1.0);
    QCOMPARE(rates[3], 1.25);
    QCOMPARE(rates[4], 1.5);
    QCOMPARE(rates[5], 2.0);

    for (const double rate : rates) {
        QVERIFY(isSupportedPlaybackRate(rate));
    }

    QVERIFY(isSupportedPlaybackRate(1.00001));
    QVERIFY(!isSupportedPlaybackRate(0.8));
    QVERIFY(!isSupportedPlaybackRate(2.5));
}

void AudioEffectsTypesTest::equalizerBandsUsePlannedFiveBandFrequencies()
{
    const EqualizerFrequencyList frequencies = defaultEqualizerCenterFrequencies();

    QCOMPARE(frequencies.size(), std::size_t{5});
    QCOMPARE(frequencies[0], 60.0);
    QCOMPARE(frequencies[1], 230.0);
    QCOMPARE(frequencies[2], 910.0);
    QCOMPARE(frequencies[3], 3600.0);
    QCOMPARE(frequencies[4], 14000.0);

    QVERIFY(isValidEqualizerBandIndex(0));
    QVERIFY(isValidEqualizerBandIndex(4));
    QVERIFY(!isValidEqualizerBandIndex(-1));
    QVERIFY(!isValidEqualizerBandIndex(5));

    const EqualizerBand band = defaultEqualizerBand(3);
    QCOMPARE(band.index, 3);
    QCOMPARE(band.centerFrequencyHz, 3600.0);
    QCOMPARE(band.gainDb, 0.0);

    const EqualizerBand invalid = defaultEqualizerBand(99);
    QCOMPARE(invalid.index, -1);
    QCOMPARE(invalid.centerFrequencyHz, 0.0);
    QCOMPARE(invalid.gainDb, 0.0);
}

void AudioEffectsTypesTest::equalizerGainRangeIsStableAndClampable()
{
    QCOMPARE(AudioEffectsMinimumGainDb, -12.0);
    QCOMPARE(AudioEffectsMaximumGainDb, 12.0);

    QVERIFY(isValidEqualizerGain(-12.0));
    QVERIFY(isValidEqualizerGain(0.0));
    QVERIFY(isValidEqualizerGain(12.0));
    QVERIFY(!isValidEqualizerGain(-12.1));
    QVERIFY(!isValidEqualizerGain(12.1));

    QCOMPARE(clampedEqualizerGain(-99.0), -12.0);
    QCOMPARE(clampedEqualizerGain(7.5), 7.5);
    QCOMPARE(clampedEqualizerGain(99.0), 12.0);
}

void AudioEffectsTypesTest::equalizerPresetsHaveStableGainMaps()
{
    compareGains(presetGains(EqualizerPreset::Flat), {0.0, 0.0, 0.0, 0.0, 0.0});
    compareGains(presetGains(EqualizerPreset::BassBoost), {6.0, 4.0, 1.0, 0.0, 0.0});
    compareGains(presetGains(EqualizerPreset::Vocal), {-2.0, 0.0, 4.0, 3.0, -1.0});
    compareGains(presetGains(EqualizerPreset::Treble), {0.0, 0.0, 1.0, 4.0, 6.0});
}

void AudioEffectsTypesTest::defaultSettingsAreSessionDefaults()
{
    const EqualizerSettings settings = defaultEqualizerSettings();

    QVERIFY(!settings.enabled);
    QVERIFY(settings.preset == EqualizerPreset::Flat);
    compareGains(settings.gainsDb, flatEqualizerGains());
}

void AudioEffectsTypesTest::capabilitiesRepresentSupportedAndUnavailableBackends()
{
    const AudioEffectsCapabilities supported = defaultAudioEffectsCapabilities();
    QVERIFY(supported.supportsPitchPreservingRate);
    QVERIFY(supported.supportsEqualizer);
    QVERIFY(supported.unavailableReason.isEmpty());
    QCOMPARE(supported.availableRates[2], AudioEffectsDefaultPlaybackRate);

    const AudioEffectsCapabilities unavailable = unavailableAudioEffectsCapabilities(QStringLiteral("GStreamer effects backend is unavailable."));
    QVERIFY(!unavailable.supportsPitchPreservingRate);
    QVERIFY(!unavailable.supportsEqualizer);
    QCOMPARE(unavailable.unavailableReason, QStringLiteral("GStreamer effects backend is unavailable."));
    QCOMPARE(unavailable.availableRates[0], 0.5);
}

void AudioEffectsTypesTest::labelsAreStableForViewModels()
{
    QCOMPARE(toString(EqualizerPreset::Flat), QStringLiteral("flat"));
    QCOMPARE(toString(EqualizerPreset::BassBoost), QStringLiteral("bass_boost"));
    QCOMPARE(toString(EqualizerPreset::Vocal), QStringLiteral("vocal"));
    QCOMPARE(toString(EqualizerPreset::Treble), QStringLiteral("treble"));

    QCOMPARE(displayLabel(EqualizerPreset::Flat), QStringLiteral("Flat"));
    QCOMPARE(displayLabel(EqualizerPreset::BassBoost), QStringLiteral("Bass Boost"));
    QCOMPARE(displayLabel(EqualizerPreset::Vocal), QStringLiteral("Vocal"));
    QCOMPARE(displayLabel(EqualizerPreset::Treble), QStringLiteral("Treble"));

    QCOMPARE(displayLabelForPlaybackRate(0.5), QStringLiteral("0.5x"));
    QCOMPARE(displayLabelForPlaybackRate(1.25), QStringLiteral("1.25x"));
    QCOMPARE(displayLabelForPlaybackRate(2.0), QStringLiteral("2x"));
}

QTEST_MAIN(AudioEffectsTypesTest)
#include "audio_effects_types_test.moc"
