#include "Model/Service/GStreamerEffectsPlaybackBackend.h"

#include <QtTest/QtTest>

#include <QSignalSpy>
#include <memory>

using namespace AudioPlayer::Model::Service;

class GStreamerEffectsBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void adaptersExposeSharedPlaybackAndEffectsState();
    void playWithoutSourceReportsRecoverableError();
    void invalidEffectsInputReportsErrorWithoutChangingState();
};

void GStreamerEffectsBackendTest::initTestCase()
{
    qRegisterMetaType<PlaybackBackendState>();
    qRegisterMetaType<PlaybackBackendError>();
}

void GStreamerEffectsBackendTest::adaptersExposeSharedPlaybackAndEffectsState()
{
    auto core = std::make_shared<GStreamerEffectsBackendCore>();
    GStreamerPlaybackService playback(core);
    GStreamerAudioEffectsService effects(core);
    QSignalSpy sourceSpy(&playback, &GStreamerPlaybackService::sourceChanged);
    QSignalSpy volumeSpy(&playback, &GStreamerPlaybackService::volumeChanged);
    QSignalSpy mutedSpy(&playback, &GStreamerPlaybackService::mutedChanged);
    QSignalSpy rateSpy(&effects, &GStreamerAudioEffectsService::playbackRateChanged);
    QSignalSpy eqSpy(&effects, &GStreamerAudioEffectsService::equalizerSettingsChanged);

    playback.setSource(QStringLiteral("/tmp/song.mp3"));
    playback.setVolume(0.25F);
    playback.setMuted(true);
    effects.setPlaybackRate(1.25);
    effects.setEqualizerEnabled(true);
    effects.applyEqualizerPreset(EqualizerPreset::BassBoost);

    QCOMPARE(playback.source(), QStringLiteral("/tmp/song.mp3"));
    QCOMPARE(sourceSpy.count(), 1);
    QCOMPARE(playback.volume(), 0.25F);
    QVERIFY(playback.muted());
    QCOMPARE(volumeSpy.count(), 1);
    QCOMPARE(mutedSpy.count(), 1);
    QVERIFY(effects.capabilities().supportsPitchPreservingRate);
    QVERIFY(effects.capabilities().supportsEqualizer);
    QCOMPARE(effects.playbackRate(), 1.25);
    QVERIFY(effects.equalizerSettings().preset == EqualizerPreset::BassBoost);
    QVERIFY(effects.equalizerSettings().enabled);
    QCOMPARE(rateSpy.count(), 1);
    QVERIFY(eqSpy.count() >= 2);
}

void GStreamerEffectsBackendTest::playWithoutSourceReportsRecoverableError()
{
    auto core = std::make_shared<GStreamerEffectsBackendCore>();
    GStreamerPlaybackService playback(core);
    QSignalSpy stateSpy(&playback, &GStreamerPlaybackService::stateChanged);
    QSignalSpy errorSpy(&playback, &GStreamerPlaybackService::playbackError);

    playback.play();

    QCOMPARE(playback.state(), PlaybackBackendState::Error);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).value<PlaybackBackendError>(), PlaybackBackendError::Resource);
    QCOMPARE(stateSpy.count(), 1);
}

void GStreamerEffectsBackendTest::invalidEffectsInputReportsErrorWithoutChangingState()
{
    auto core = std::make_shared<GStreamerEffectsBackendCore>();
    GStreamerAudioEffectsService effects(core);
    QSignalSpy errorSpy(&effects, &GStreamerAudioEffectsService::audioEffectsError);
    QSignalSpy rateSpy(&effects, &GStreamerAudioEffectsService::playbackRateChanged);
    QSignalSpy eqSpy(&effects, &GStreamerAudioEffectsService::equalizerSettingsChanged);

    effects.setPlaybackRate(0.8);
    QCOMPARE(effects.playbackRate(), 1.0);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(rateSpy.count(), 0);

    effects.setEqualizerBandGain(9, 1.0);
    QCOMPARE(errorSpy.count(), 2);
    QCOMPARE(eqSpy.count(), 0);
}

QTEST_MAIN(GStreamerEffectsBackendTest)
#include "gstreamer_effects_backend_test.moc"
