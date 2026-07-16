#include "Model/Service/AudioEffectsUseCase.h"
#include "Model/Service/NullAudioEffectsService.h"

#include <QtTest/QtTest>

using namespace AudioPlayer::Model::Service;

class NullAudioEffectsServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void reportsUnavailableCapabilitiesAndRecoverableErrors();
};

void NullAudioEffectsServiceTest::reportsUnavailableCapabilitiesAndRecoverableErrors()
{
    const QString reason = QStringLiteral("GStreamer effects backend unavailable.");
    NullAudioEffectsService service(reason);
    AudioEffectsUseCase useCase(&service);
    QSignalSpy serviceErrorSpy(&service, &NullAudioEffectsService::audioEffectsError);
    QSignalSpy useCaseErrorSpy(&useCase, &AudioEffectsUseCase::lastErrorChanged);

    QVERIFY(!service.capabilities().supportsPitchPreservingRate);
    QVERIFY(!service.capabilities().supportsEqualizer);
    QCOMPARE(service.statusMessage(), reason);
    QCOMPARE(service.lastError(), reason);
    QCOMPARE(useCase.statusMessage(), reason);
    QCOMPARE(useCase.lastError(), reason);

    service.setPlaybackRate(1.25);
    QCOMPARE(serviceErrorSpy.count(), 1);
    QCOMPARE(serviceErrorSpy.takeFirst().at(0).toString(), reason);

    QVERIFY(!useCase.setEqualizerEnabled(true));
    QVERIFY(useCase.lastError().contains(reason));
    QVERIFY(useCaseErrorSpy.count() <= 1);
}

QTEST_MAIN(NullAudioEffectsServiceTest)
#include "null_audio_effects_service_test.moc"
