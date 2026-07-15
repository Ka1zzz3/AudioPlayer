#include "Model/AudioFile.h"
#include "Model/Service/IPlaybackService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "ViewModel/PlaybackViewModel.h"

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
using AudioPlayer::ViewModel::PlaybackState;
using AudioPlayer::ViewModel::PlaybackViewModel;

class PlaybackViewModelFakeService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit PlaybackViewModelFakeService(QObject *parent = nullptr)
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
    void stop() override { setState(PlaybackBackendState::Stopped); }

    void seekTo(qint64 positionMs) override
    {
        m_positionMs = std::max(positionMs, qint64{0});
        emit positionChanged(m_positionMs);
    }

    void setVolume(float volume) override
    {
        const float clamped = std::clamp(volume, 0.0F, 1.0F);
        if (qFuzzyCompare(m_volume, clamped)) {
            return;
        }

        m_volume = clamped;
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
        m_durationMs = std::max(durationMs, qint64{0});
        emit durationChanged(m_durationMs);
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

    void failPlayback(QString message)
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
};

class PlaybackViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void commandsExposePlaybackIntentAndState();
    void progressSeekVolumeAndMuteReflectBackendState();
    void missingFileErrorRemainsVisibleAndRecoveryClearsIt();
    void backendErrorAutoNextUpdatesCurrentTrack();
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

void PlaybackViewModelTest::initTestCase()
{
    qRegisterMetaType<PlaybackState>();
    qRegisterMetaType<PlaybackBackendState>();
    qRegisterMetaType<PlaybackBackendError>();
}

void PlaybackViewModelTest::commandsExposePlaybackIntentAndState()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString firstPath = createAudioFile(temporaryDirectory, QStringLiteral("first.mp3"));
    const QString secondPath = createAudioFile(temporaryDirectory, QStringLiteral("second.mp3"));
    PlaybackViewModelFakeService service;
    PlaybackUseCase useCase(service);
    PlaybackViewModel viewModel(useCase, service);
    QSignalSpy stateSpy(&viewModel, &PlaybackViewModel::playbackStateChanged);
    QSignalSpy currentSpy(&viewModel, &PlaybackViewModel::currentPlaybackIndexChanged);

    viewModel.replaceQueue(queue({track(firstPath, QStringLiteral("First")), track(secondPath, QStringLiteral("Second"))}));

    QVERIFY(viewModel.playCommand()->execute());
    QCOMPARE(viewModel.playbackState(), PlaybackState::Playing);
    QCOMPARE(viewModel.currentPlaybackIndex(), 0);
    QCOMPARE(viewModel.currentPlaybackTitle(), QStringLiteral("First"));

    QVERIFY(viewModel.nextCommand()->execute());
    QCOMPARE(viewModel.currentPlaybackIndex(), 1);
    QCOMPARE(viewModel.currentPlaybackTitle(), QStringLiteral("Second"));

    QVERIFY(viewModel.previousCommand()->execute());
    QCOMPARE(viewModel.currentPlaybackIndex(), 0);

    QVERIFY(viewModel.pauseCommand()->execute());
    QCOMPARE(viewModel.playbackState(), PlaybackState::Paused);

    QVERIFY(viewModel.stopCommand()->execute());
    QCOMPARE(viewModel.playbackState(), PlaybackState::Stopped);
    QVERIFY(stateSpy.count() >= 3);
    QVERIFY(currentSpy.count() >= 3);
}

void PlaybackViewModelTest::progressSeekVolumeAndMuteReflectBackendState()
{
    PlaybackViewModelFakeService service;
    PlaybackUseCase useCase(service);
    PlaybackViewModel viewModel(useCase, service);
    QSignalSpy positionSpy(&viewModel, &PlaybackViewModel::positionMsChanged);
    QSignalSpy durationSpy(&viewModel, &PlaybackViewModel::durationMsChanged);
    QSignalSpy volumeSpy(&viewModel, &PlaybackViewModel::volumePercentChanged);
    QSignalSpy mutedSpy(&viewModel, &PlaybackViewModel::mutedChanged);

    service.setDuration(90'000);
    service.seekTo(5'000);
    service.setSeekable(true);
    viewModel.seekTo(12'000);
    viewModel.setVolumePercent(25);
    QVERIFY(viewModel.toggleMuteCommand()->execute());

    QCOMPARE(viewModel.durationMs(), 90'000);
    QCOMPARE(viewModel.positionMs(), 12'000);
    QVERIFY(viewModel.seekable());
    QCOMPARE(viewModel.volumePercent(), 25);
    QVERIFY(viewModel.muted());
    QCOMPARE(positionSpy.count(), 2);
    QCOMPARE(durationSpy.count(), 1);
    QCOMPARE(volumeSpy.count(), 1);
    QCOMPARE(mutedSpy.count(), 1);
}

void PlaybackViewModelTest::missingFileErrorRemainsVisibleAndRecoveryClearsIt()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString missingPath = temporaryDirectory.filePath(QStringLiteral("missing.mp3"));
    const QString playablePath = createAudioFile(temporaryDirectory, QStringLiteral("playable.mp3"));
    PlaybackViewModelFakeService service;
    PlaybackUseCase useCase(service);
    PlaybackViewModel viewModel(useCase, service);
    QSignalSpy errorSpy(&viewModel, &PlaybackViewModel::lastPlaybackErrorChanged);

    viewModel.replaceQueue(queue({track(missingPath, QStringLiteral("Missing")), track(playablePath, QStringLiteral("Playable"))}));
    QVERIFY(viewModel.playCommand()->execute());

    QCOMPARE(viewModel.currentPlaybackIndex(), 1);
    QCOMPARE(viewModel.currentPlaybackTitle(), QStringLiteral("Playable"));
    QVERIFY(viewModel.lastPlaybackError().contains(QStringLiteral("Missing")));
    QVERIFY(viewModel.statusMessage().contains(QStringLiteral("Missing")));

    QVERIFY(viewModel.playCommand()->execute());
    QCOMPARE(viewModel.lastPlaybackError(), QString());
    QVERIFY(errorSpy.count() >= 2);
}

void PlaybackViewModelTest::backendErrorAutoNextUpdatesCurrentTrack()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString firstPath = createAudioFile(temporaryDirectory, QStringLiteral("first.mp3"));
    const QString secondPath = createAudioFile(temporaryDirectory, QStringLiteral("second.mp3"));
    PlaybackViewModelFakeService service;
    PlaybackUseCase useCase(service);
    PlaybackViewModel viewModel(useCase, service);
    viewModel.replaceQueue(queue({track(firstPath, QStringLiteral("First")), track(secondPath, QStringLiteral("Second"))}));

    QVERIFY(viewModel.playCommand()->execute());
    service.failPlayback(QStringLiteral("Cannot decode First"));

    QCOMPARE(viewModel.playbackState(), PlaybackState::Playing);
    QCOMPARE(viewModel.currentPlaybackIndex(), 1);
    QCOMPARE(viewModel.currentPlaybackTitle(), QStringLiteral("Second"));
    QVERIFY(viewModel.lastPlaybackError().contains(QStringLiteral("First")));

    service.finishPlayback();
    QCOMPARE(viewModel.playbackState(), PlaybackState::Stopped);
    QVERIFY(viewModel.statusMessage().contains(QStringLiteral("stopped"), Qt::CaseInsensitive)
            || viewModel.statusMessage().contains(QStringLiteral("end"), Qt::CaseInsensitive));
}

QTEST_MAIN(PlaybackViewModelTest)

#include "playback_viewmodel_test.moc"
