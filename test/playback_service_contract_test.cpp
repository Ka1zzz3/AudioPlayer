#include "Model/Service/IPlaybackService.h"

#include <QtTest/QtTest>

#include <algorithm>

using AudioPlayer::Model::Service::IPlaybackService;
using AudioPlayer::Model::Service::PlaybackBackendError;
using AudioPlayer::Model::Service::PlaybackBackendErrorInfo;
using AudioPlayer::Model::Service::PlaybackBackendState;

class FakePlaybackService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit FakePlaybackService(QObject *parent = nullptr)
        : IPlaybackService(parent)
    {
    }

    [[nodiscard]] QString source() const override { return m_source; }
    [[nodiscard]] PlaybackBackendState state() const noexcept override { return m_state; }
    [[nodiscard]] qint64 positionMs() const noexcept override { return m_positionMs; }
    [[nodiscard]] qint64 durationMs() const noexcept override { return m_durationMs; }
    [[nodiscard]] bool seekable() const noexcept override { return m_seekable; }
    [[nodiscard]] float volume() const noexcept override { return m_volume; }
    [[nodiscard]] bool muted() const noexcept override { return m_muted; }

public slots:
    void setSource(QString filePath) override
    {
        if (m_source == filePath) {
            return;
        }

        m_source = std::move(filePath);
        emit sourceChanged(m_source);
    }

    void play() override { setState(PlaybackBackendState::Playing); }
    void pause() override { setState(PlaybackBackendState::Paused); }

    void stop() override
    {
        setState(PlaybackBackendState::Stopped);
        seekTo(0);
    }

    void seekTo(qint64 positionMs) override
    {
        const qint64 clampedPosition = std::clamp(positionMs, qint64{0}, m_durationMs);
        if (m_positionMs == clampedPosition) {
            return;
        }

        m_positionMs = clampedPosition;
        emit positionChanged(m_positionMs);
    }

    void setVolume(float volume) override
    {
        const float clampedVolume = std::clamp(volume, 0.0F, 1.0F);
        if (qFuzzyCompare(m_volume, clampedVolume)) {
            return;
        }

        m_volume = clampedVolume;
        emit volumeChanged(m_volume);
    }

    void setMuted(bool muted) override
    {
        if (m_muted == muted) {
            return;
        }

        m_muted = muted;
        emit mutedChanged(m_muted);
    }

    void setDuration(qint64 durationMs)
    {
        const qint64 normalizedDuration = std::max(durationMs, qint64{0});
        if (m_durationMs == normalizedDuration) {
            return;
        }

        m_durationMs = normalizedDuration;
        emit durationChanged(m_durationMs);
    }

    void setSeekable(bool seekable)
    {
        if (m_seekable == seekable) {
            return;
        }

        m_seekable = seekable;
        emit seekableChanged(m_seekable);
    }

    void finishPlayback()
    {
        setState(PlaybackBackendState::Ended);
        emit playbackEnded();
    }

    void fail(PlaybackBackendError error, QString message)
    {
        m_lastError = PlaybackBackendErrorInfo{error, message};
        setState(PlaybackBackendState::Error);
        emit playbackError(m_lastError.code, m_lastError.message);
    }

private:
    void setState(PlaybackBackendState state)
    {
        if (m_state == state) {
            return;
        }

        m_state = state;
        emit stateChanged(m_state);
    }

    QString m_source;
    PlaybackBackendState m_state = PlaybackBackendState::Stopped;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_seekable = false;
    float m_volume = 1.0F;
    bool m_muted = false;
    PlaybackBackendErrorInfo m_lastError;
};

class PlaybackServiceContractTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void fakePlaybackServiceEmitsStatePositionDurationAndEnd();
    void fakePlaybackServiceEmitsRecoverableError();
    void fakePlaybackServiceClampsVolumeAndTracksMuteSeparately();
};

void PlaybackServiceContractTest::initTestCase()
{
    qRegisterMetaType<PlaybackBackendState>();
    qRegisterMetaType<PlaybackBackendError>();
    qRegisterMetaType<PlaybackBackendErrorInfo>();
}

void PlaybackServiceContractTest::fakePlaybackServiceEmitsStatePositionDurationAndEnd()
{
    FakePlaybackService service;
    QSignalSpy sourceSpy(&service, &FakePlaybackService::sourceChanged);
    QSignalSpy stateSpy(&service, &FakePlaybackService::stateChanged);
    QSignalSpy durationSpy(&service, &FakePlaybackService::durationChanged);
    QSignalSpy positionSpy(&service, &FakePlaybackService::positionChanged);
    QSignalSpy endedSpy(&service, &FakePlaybackService::playbackEnded);

    service.setSource(QStringLiteral("/music/song.mp3"));
    service.setDuration(42'000);
    service.setSeekable(true);
    service.play();
    service.seekTo(5'000);
    service.finishPlayback();

    QCOMPARE(service.source(), QStringLiteral("/music/song.mp3"));
    QCOMPARE(service.durationMs(), 42'000);
    QCOMPARE(service.positionMs(), 5'000);
    QVERIFY(service.seekable());
    QCOMPARE(service.state(), PlaybackBackendState::Ended);
    QCOMPARE(sourceSpy.count(), 1);
    QCOMPARE(durationSpy.count(), 1);
    QCOMPARE(positionSpy.count(), 1);
    QCOMPARE(endedSpy.count(), 1);
    QVERIFY(stateSpy.count() >= 2);
}

void PlaybackServiceContractTest::fakePlaybackServiceEmitsRecoverableError()
{
    FakePlaybackService service;
    QSignalSpy stateSpy(&service, &FakePlaybackService::stateChanged);
    QSignalSpy errorSpy(&service, &FakePlaybackService::playbackError);

    service.setSource(QStringLiteral("/music/unsupported.flac"));
    service.fail(PlaybackBackendError::Format, QStringLiteral("Unsupported format"));

    QCOMPARE(service.state(), PlaybackBackendState::Error);
    QCOMPARE(errorSpy.count(), 1);
    QCOMPARE(errorSpy.takeFirst().at(0).value<PlaybackBackendError>(), PlaybackBackendError::Format);
    QVERIFY(stateSpy.count() >= 1);
}

void PlaybackServiceContractTest::fakePlaybackServiceClampsVolumeAndTracksMuteSeparately()
{
    FakePlaybackService service;
    QSignalSpy volumeSpy(&service, &FakePlaybackService::volumeChanged);
    QSignalSpy mutedSpy(&service, &FakePlaybackService::mutedChanged);

    service.setVolume(0.25F);
    service.setMuted(true);
    service.setVolume(1.5F);
    service.setVolume(-1.0F);
    service.setMuted(false);

    QCOMPARE(service.volume(), 0.0F);
    QVERIFY(!service.muted());
    QCOMPARE(volumeSpy.count(), 3);
    QCOMPARE(mutedSpy.count(), 2);
}

QTEST_MAIN(PlaybackServiceContractTest)

#include "playback_service_contract_test.moc"
