#include "Model/Service/QtMultimediaPlaybackService.h"

#include <QtTest/QtTest>

#include <QSignalSpy>
#include <QTemporaryDir>

using AudioPlayer::Model::Service::PlaybackBackendError;
using AudioPlayer::Model::Service::PlaybackBackendState;
using AudioPlayer::Model::Service::QtMultimediaPlaybackService;

class QtMultimediaPlaybackServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void setSourceVolumeAndMuteEmitBackendSignals();
    void playWithoutSourceReportsRecoverableError();
};

void QtMultimediaPlaybackServiceTest::initTestCase()
{
    qRegisterMetaType<PlaybackBackendState>();
    qRegisterMetaType<PlaybackBackendError>();
}

void QtMultimediaPlaybackServiceTest::setSourceVolumeAndMuteEmitBackendSignals()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString sourcePath = temporaryDirectory.filePath(QStringLiteral("song.mp3"));
    QtMultimediaPlaybackService service;
    QSignalSpy sourceSpy(&service, &QtMultimediaPlaybackService::sourceChanged);
    QSignalSpy volumeSpy(&service, &QtMultimediaPlaybackService::volumeChanged);
    QSignalSpy mutedSpy(&service, &QtMultimediaPlaybackService::mutedChanged);

    service.setSource(sourcePath);
    service.setVolume(0.25F);
    service.setMuted(true);

    QCOMPARE(service.source(), sourcePath);
    QCOMPARE(sourceSpy.count(), 1);
    QCOMPARE(sourceSpy.takeFirst().at(0).toString(), sourcePath);
    QCOMPARE(service.volume(), 0.25F);
    QVERIFY(service.muted());
    QVERIFY(volumeSpy.count() <= 1);
    QCOMPARE(mutedSpy.count(), 1);
}

void QtMultimediaPlaybackServiceTest::playWithoutSourceReportsRecoverableError()
{
    QtMultimediaPlaybackService service;
    QSignalSpy stateSpy(&service, &QtMultimediaPlaybackService::stateChanged);
    QSignalSpy errorSpy(&service, &QtMultimediaPlaybackService::playbackError);

    service.play();

    QCOMPARE(service.state(), PlaybackBackendState::Error);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).value<PlaybackBackendError>(), PlaybackBackendError::Resource);
    QCOMPARE(stateSpy.count(), 1);
}

QTEST_MAIN(QtMultimediaPlaybackServiceTest)

#include "qt_multimedia_playback_service_test.moc"
