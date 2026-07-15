#include "Model/Service/PlaybackUseCase.h"

#include <QtTest/QtTest>

#include <algorithm>

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::Service::IPlaybackService;
using AudioPlayer::Model::Service::PlaybackBackendError;
using AudioPlayer::Model::Service::PlaybackBackendState;
using AudioPlayer::Model::Service::PlaybackUseCase;

class PlaybackUseCaseFakeService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit PlaybackUseCaseFakeService(QObject *parent = nullptr)
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
    [[nodiscard]] int playCount() const noexcept { return m_playCount; }
    [[nodiscard]] int stopCount() const noexcept { return m_stopCount; }

public slots:
    void setSource(QString filePath) override
    {
        if (m_source == filePath) {
            return;
        }

        m_source = std::move(filePath);
        emit sourceChanged(m_source);
    }

    void play() override
    {
        ++m_playCount;
        setState(PlaybackBackendState::Playing);
    }

    void pause() override { setState(PlaybackBackendState::Paused); }

    void stop() override
    {
        ++m_stopCount;
        setState(PlaybackBackendState::Stopped);
    }

    void seekTo(qint64 positionMs) override
    {
        m_positionMs = positionMs;
        emit positionChanged(m_positionMs);
    }

    void setVolume(float volume) override
    {
        m_volume = std::clamp(volume, 0.0F, 1.0F);
        emit volumeChanged(m_volume);
    }

    void setMuted(bool muted) override
    {
        m_muted = muted;
        emit mutedChanged(m_muted);
    }

    void setSeekable(bool seekable)
    {
        m_seekable = seekable;
        emit seekableChanged(m_seekable);
    }

    void finishPlayback()
    {
        setState(PlaybackBackendState::Ended);
        emit playbackEnded();
    }

    void failPlayback(QString message = QStringLiteral("Backend rejected file"))
    {
        setState(PlaybackBackendState::Error);
        emit playbackError(PlaybackBackendError::Format, message);
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
    int m_playCount = 0;
    int m_stopCount = 0;
};

class PlaybackUseCaseTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void playStartsSelectedOrFirstQueueItem();
    void missingSelectedTrackReportsErrorAndSkipsForward();
    void nextPreviousAndEndOfMediaFollowQueueOrder();
    void backendErrorSkipsForwardAndStopsWhenNoPlayableTracksRemain();
    void emptyQueueStopsWithVisibleError();
    void replacingQueueClearsMissingCurrentSource();
    void seekVolumeAndMuteDelegateSafely();
};

namespace {

QString createAudioFile(const QTemporaryDir &temporaryDirectory, const QString &fileName)
{
    const QString filePath = temporaryDirectory.filePath(fileName);
    QFile file(filePath);
    const bool opened = file.open(QIODevice::WriteOnly);
    Q_ASSERT(opened);
    Q_UNUSED(opened)
    const qint64 written = file.write("fake audio bytes");
    Q_ASSERT(written > 0);
    Q_UNUSED(written)
    file.close();
    return QFileInfo(filePath).absoluteFilePath();
}

AudioFile track(const QString &path, const QString &title)
{
    return AudioFile(path, title, QStringLiteral("Artist"), QStringLiteral("Album"), 1);
}

QVector<AudioFile> queue(std::initializer_list<AudioFile> items)
{
    QVector<AudioFile> result;
    result.reserve(static_cast<int>(items.size()));
    for (const AudioFile &item : items) {
        result.push_back(item);
    }
    return result;
}

} // namespace

void PlaybackUseCaseTest::initTestCase()
{
    qRegisterMetaType<PlaybackBackendState>();
    qRegisterMetaType<PlaybackBackendError>();
}

void PlaybackUseCaseTest::playStartsSelectedOrFirstQueueItem()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString firstPath = createAudioFile(temporaryDirectory, QStringLiteral("first.mp3"));
    const QString secondPath = createAudioFile(temporaryDirectory, QStringLiteral("second.mp3"));
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);
    QSignalSpy currentSpy(&useCase, &PlaybackUseCase::currentTrackChanged);

    useCase.setQueue(queue({track(firstPath, QStringLiteral("First")), track(secondPath, QStringLiteral("Second"))}));
    useCase.play();

    QCOMPARE(useCase.currentIndex(), 0);
    QCOMPARE(service.source(), firstPath);
    QCOMPARE(service.state(), PlaybackBackendState::Playing);

    QVERIFY(useCase.setCurrentIndex(1));
    useCase.play();

    QCOMPARE(useCase.currentIndex(), 1);
    QCOMPARE(service.source(), secondPath);
    QVERIFY(currentSpy.count() >= 2);
}

void PlaybackUseCaseTest::missingSelectedTrackReportsErrorAndSkipsForward()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString missingPath = temporaryDirectory.filePath(QStringLiteral("missing.mp3"));
    const QString playablePath = createAudioFile(temporaryDirectory, QStringLiteral("playable.mp3"));
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);
    QSignalSpy errorSpy(&useCase, &PlaybackUseCase::playbackErrorOccurred);

    useCase.setQueue(queue({track(missingPath, QStringLiteral("Missing")), track(playablePath, QStringLiteral("Playable"))}));
    useCase.play();

    QCOMPARE(useCase.currentIndex(), 1);
    QCOMPARE(service.source(), playablePath);
    QCOMPARE(service.state(), PlaybackBackendState::Playing);
    QCOMPARE(errorSpy.count(), 1);
    QVERIFY(errorSpy.takeFirst().at(0).toString().contains(QStringLiteral("Missing")));
}

void PlaybackUseCaseTest::nextPreviousAndEndOfMediaFollowQueueOrder()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString firstPath = createAudioFile(temporaryDirectory, QStringLiteral("first.mp3"));
    const QString secondPath = createAudioFile(temporaryDirectory, QStringLiteral("second.mp3"));
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);
    useCase.setQueue(queue({track(firstPath, QStringLiteral("First")), track(secondPath, QStringLiteral("Second"))}));

    useCase.play();
    useCase.next();
    QCOMPARE(useCase.currentIndex(), 1);
    QCOMPARE(service.source(), secondPath);

    useCase.previous();
    QCOMPARE(useCase.currentIndex(), 0);
    QCOMPARE(service.source(), firstPath);

    service.finishPlayback();
    QCOMPARE(useCase.currentIndex(), 1);
    QCOMPARE(service.source(), secondPath);

    service.finishPlayback();
    QCOMPARE(service.state(), PlaybackBackendState::Stopped);
    QCOMPARE(service.stopCount(), 1);
}

void PlaybackUseCaseTest::backendErrorSkipsForwardAndStopsWhenNoPlayableTracksRemain()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString firstPath = createAudioFile(temporaryDirectory, QStringLiteral("first.mp3"));
    const QString secondPath = createAudioFile(temporaryDirectory, QStringLiteral("second.mp3"));
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);
    QSignalSpy errorSpy(&useCase, &PlaybackUseCase::playbackErrorOccurred);
    useCase.setQueue(queue({track(firstPath, QStringLiteral("First")), track(secondPath, QStringLiteral("Second"))}));

    useCase.play();
    service.failPlayback(QStringLiteral("Cannot decode first"));

    QCOMPARE(useCase.currentIndex(), 1);
    QCOMPARE(service.source(), secondPath);
    QCOMPARE(service.state(), PlaybackBackendState::Playing);

    service.failPlayback(QStringLiteral("Cannot decode second"));

    QCOMPARE(service.state(), PlaybackBackendState::Stopped);
    QVERIFY(errorSpy.count() >= 3);
    QVERIFY(errorSpy.at(0).at(0).toString().contains(QStringLiteral("first"), Qt::CaseInsensitive));
}

void PlaybackUseCaseTest::emptyQueueStopsWithVisibleError()
{
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);
    QSignalSpy errorSpy(&useCase, &PlaybackUseCase::playbackErrorOccurred);
    QSignalSpy stoppedSpy(&useCase, &PlaybackUseCase::playbackStopped);

    useCase.setQueue({});
    useCase.play();

    QCOMPARE(useCase.currentIndex(), -1);
    QCOMPARE(service.state(), PlaybackBackendState::Stopped);
    QVERIFY(errorSpy.count() >= 1);
    QVERIFY(stoppedSpy.count() >= 1);
}

void PlaybackUseCaseTest::replacingQueueClearsMissingCurrentSource()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString oldPath = createAudioFile(temporaryDirectory, QStringLiteral("old.mp3"));
    const QString newPath = createAudioFile(temporaryDirectory, QStringLiteral("new.mp3"));
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);
    QSignalSpy currentIndexSpy(&useCase, &PlaybackUseCase::currentIndexChanged);

    useCase.setQueue(queue({track(oldPath, QStringLiteral("Old"))}));
    useCase.play();
    useCase.setQueue(queue({track(newPath, QStringLiteral("New"))}));

    QCOMPARE(useCase.currentIndex(), -1);
    QCOMPARE(service.source(), QString());
    QCOMPARE(service.state(), PlaybackBackendState::Stopped);
    QVERIFY(currentIndexSpy.count() >= 2);
}

void PlaybackUseCaseTest::seekVolumeAndMuteDelegateSafely()
{
    PlaybackUseCaseFakeService service;
    PlaybackUseCase useCase(service);

    useCase.seekTo(1234);
    QCOMPARE(service.positionMs(), 0);

    service.setSeekable(true);
    useCase.seekTo(1234);
    useCase.setVolume(0.25F);
    useCase.setMuted(true);

    QCOMPARE(service.positionMs(), 1234);
    QCOMPARE(service.volume(), 0.25F);
    QVERIFY(service.muted());
}

QTEST_MAIN(PlaybackUseCaseTest)

#include "playback_usecase_test.moc"
