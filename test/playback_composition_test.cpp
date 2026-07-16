#include "Model/AudioFile.h"
#include "Model/JsonSongRepository.h"
#include "Model/PlayList.h"
#include "Model/Service/IPlaybackService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "Model/Service/PlaylistCollectionUseCase.h"
#include "Model/Service/TranscodedPlaylistService.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"

#include <QtTest/QtTest>

#include <algorithm>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::JsonSongRepository;
using AudioPlayer::Model::PlayList;
using AudioPlayer::Model::Service::IPlaybackService;
using AudioPlayer::Model::Service::PlaybackBackendError;
using AudioPlayer::Model::Service::PlaybackBackendState;
using AudioPlayer::Model::Service::PlaybackUseCase;
using AudioPlayer::Model::Service::PlaylistCollectionUseCase;
using AudioPlayer::Model::Service::TranscodedPlaylistService;
using AudioPlayer::ViewModel::LibraryViewModel;
using AudioPlayer::ViewModel::PlaybackState;
using AudioPlayer::ViewModel::PlaybackViewModel;
using AudioPlayer::ViewModel::PlaylistCollectionViewModel;

class CompositionFakePlaybackService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit CompositionFakePlaybackService(QObject *parent = nullptr)
        : IPlaybackService(parent)
    {
    }

    [[nodiscard]] QString source() const override { return m_source; }
    [[nodiscard]] PlaybackBackendState state() const noexcept override { return m_state; }
    [[nodiscard]] qint64 positionMs() const noexcept override { return 0; }
    [[nodiscard]] qint64 durationMs() const noexcept override { return 0; }
    [[nodiscard]] bool seekable() const noexcept override { return false; }
    [[nodiscard]] float volume() const noexcept override { return m_volume; }
    [[nodiscard]] bool muted() const noexcept override { return m_muted; }
    [[nodiscard]] int setSourceCount() const noexcept { return m_setSourceCount; }

public slots:
    void setSource(QString filePath) override
    {
        ++m_setSourceCount;
        if (m_source == filePath) {
            return;
        }

        m_source = std::move(filePath);
        emit sourceChanged(m_source);
    }

    void play() override { setState(PlaybackBackendState::Playing); }
    void pause() override { setState(PlaybackBackendState::Paused); }
    void stop() override { setState(PlaybackBackendState::Stopped); }
    void seekTo(qint64 positionMs) override { Q_UNUSED(positionMs) }
    void setVolume(float volume) override { m_volume = std::clamp(volume, 0.0F, 1.0F); }
    void setMuted(bool muted) override { m_muted = muted; }

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
    float m_volume = 1.0F;
    bool m_muted = false;
    int m_setSourceCount = 0;
};

class PlaybackCompositionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void libraryChangesDoNotSynchronizePlaybackQueue();
    void refreshDoesNotClearPlaybackWhenCurrentSourceDisappears();
    void explicitPlayRequestReplacesPlaybackQueueAndStartsPlayback();
    void visiblePlaylistSwitchDoesNotReplaceActivePlaybackQueue();
    void transcodedPlaylistInsertionDoesNotReplacePlaybackQueueOrAutoPlay();
};

namespace {

QString createFile(const QString &filePath)
{
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

QString createAudioFile(const QTemporaryDir &temporaryDirectory, const QString &fileName)
{
    return createFile(temporaryDirectory.filePath(fileName));
}

QString savePlayList(const QTemporaryDir &temporaryDirectory, const PlayList &playList)
{
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    JsonSongRepository repository(storagePath);
    QString errorMessage;
    const bool saved = repository.save(playList, &errorMessage);
    Q_ASSERT(saved);
    Q_UNUSED(saved)
    Q_ASSERT(errorMessage.isEmpty());
    return storagePath;
}

struct PlaybackCompositionFixture
{
    LibraryViewModel libraryViewModel;
    CompositionFakePlaybackService playbackService;
    PlaybackUseCase playbackUseCase{playbackService};
    PlaybackViewModel playbackViewModel{playbackUseCase, playbackService};
    PlaylistCollectionViewModel playlistCollectionViewModel;

    PlaybackCompositionFixture()
    {
        QObject::connect(&playlistCollectionViewModel,
                         &PlaylistCollectionViewModel::playRequested,
                         &playbackViewModel,
                         [this](const QString &, const QVector<AudioFile> &songsSnapshot, int startIndex) {
                             playbackViewModel.replaceQueue(songsSnapshot);
                             playbackViewModel.setCurrentPlaybackIndex(startIndex);
                             playbackViewModel.playCommand()->execute();
                         });
    }
};

} // namespace

void PlaybackCompositionTest::initTestCase()
{
    qRegisterMetaType<PlaybackState>();
    qRegisterMetaType<PlaybackBackendState>();
    qRegisterMetaType<PlaybackBackendError>();
}

void PlaybackCompositionTest::libraryChangesDoNotSynchronizePlaybackQueue()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString scanPath = temporaryDirectory.filePath(QStringLiteral("scan"));
    QVERIFY(QDir().mkpath(scanPath));
    createFile(QDir(scanPath).filePath(QStringLiteral("Scanned.mp3")));
    PlaybackCompositionFixture fixture;
    QSignalSpy libraryChangedSpy(&fixture.libraryViewModel, &LibraryViewModel::libraryChanged);

    fixture.libraryViewModel.setScanDirectoryPath(scanPath);
    QVERIFY(fixture.libraryViewModel.scanCommand()->execute());
    fixture.playbackViewModel.playCommand()->execute();

    QCOMPARE(libraryChangedSpy.count(), 1);
    QCOMPARE(fixture.playbackViewModel.currentPlaybackIndex(), -1);
    QCOMPARE(fixture.playbackService.source(), QString());
    QCOMPARE(fixture.playbackViewModel.playbackState(), PlaybackState::Stopped);
}

void PlaybackCompositionTest::refreshDoesNotClearPlaybackWhenCurrentSourceDisappears()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString disappearingPath = createAudioFile(temporaryDirectory, QStringLiteral("Disappear.mp3"));
    const QString remainingPath = createAudioFile(temporaryDirectory, QStringLiteral("Remain.mp3"));
    PlayList playList;
    playList.add(AudioFile(disappearingPath, QStringLiteral("Disappear")));
    playList.add(AudioFile(remainingPath, QStringLiteral("Remain")));
    const QString storagePath = savePlayList(temporaryDirectory, playList);
    PlaybackCompositionFixture fixture;

    fixture.playbackViewModel.replaceQueue(playList.items());
    fixture.playbackViewModel.setCurrentPlaybackIndex(0);
    QVERIFY(fixture.playbackViewModel.playCommand()->execute());
    QCOMPARE(fixture.playbackService.source(), disappearingPath);

    fixture.libraryViewModel.setStoragePath(storagePath);
    QVERIFY(fixture.libraryViewModel.loadCommand()->execute());
    QVERIFY(QFile::remove(disappearingPath));
    QVERIFY(fixture.libraryViewModel.refreshCommand()->execute());

    QCOMPARE(fixture.libraryViewModel.count(), 1);
    QCOMPARE(fixture.playbackViewModel.currentPlaybackIndex(), 0);
    QCOMPARE(fixture.playbackService.source(), disappearingPath);
    QCOMPARE(fixture.playbackViewModel.playbackState(), PlaybackState::Playing);
}

void PlaybackCompositionTest::explicitPlayRequestReplacesPlaybackQueueAndStartsPlayback()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString firstPath = createAudioFile(temporaryDirectory, QStringLiteral("One.mp3"));
    const QString secondPath = createAudioFile(temporaryDirectory, QStringLiteral("Two.mp3"));
    PlaybackCompositionFixture fixture;
    QVector<AudioFile> songs{
        AudioFile(firstPath, QStringLiteral("One")),
        AudioFile(secondPath, QStringLiteral("Two")),
    };
    fixture.playlistCollectionViewModel.setCurrentVisibleSongs(songs);
    fixture.playlistCollectionViewModel.setSelectedSongIndex(1);

    QVERIFY(fixture.playlistCollectionViewModel.playSelectedSongCommand()->execute());

    QCOMPARE(fixture.playbackViewModel.currentPlaybackIndex(), 1);
    QCOMPARE(fixture.playbackViewModel.currentPlaybackTitle(), QStringLiteral("Two"));
    QCOMPARE(fixture.playbackService.source(), secondPath);
    QCOMPARE(fixture.playbackViewModel.playbackState(), PlaybackState::Playing);
}

void PlaybackCompositionTest::visiblePlaylistSwitchDoesNotReplaceActivePlaybackQueue()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString playingPath = createAudioFile(temporaryDirectory, QStringLiteral("Playing.mp3"));
    const QString visiblePath = createAudioFile(temporaryDirectory, QStringLiteral("Visible.mp3"));
    PlaybackCompositionFixture fixture;
    QVector<AudioFile> activeQueue{AudioFile(playingPath, QStringLiteral("Playing"))};
    QVector<AudioFile> visibleSongs{AudioFile(visiblePath, QStringLiteral("Visible"))};

    fixture.playbackViewModel.replaceQueue(activeQueue);
    fixture.playbackViewModel.setCurrentPlaybackIndex(0);
    QVERIFY(fixture.playbackViewModel.playCommand()->execute());
    const int setSourceCountBeforeSwitch = fixture.playbackService.setSourceCount();

    fixture.playlistCollectionViewModel.setNewPlaylistName(QStringLiteral("Other"));
    QVERIFY(fixture.playlistCollectionViewModel.createPlaylistCommand()->execute());
    fixture.playlistCollectionViewModel.setCurrentVisibleSongs(visibleSongs);
    fixture.playlistCollectionViewModel.setSelectedPlaylistId(QStringLiteral("playlist-2"));
    QVERIFY(fixture.playlistCollectionViewModel.switchPlaylistCommand()->execute());

    QCOMPARE(fixture.playbackViewModel.currentPlaybackIndex(), 0);
    QCOMPARE(fixture.playbackViewModel.currentPlaybackTitle(), QStringLiteral("Playing"));
    QCOMPARE(fixture.playbackService.source(), playingPath);
    QCOMPARE(fixture.playbackService.setSourceCount(), setSourceCountBeforeSwitch);
    QCOMPARE(fixture.playbackViewModel.playbackState(), PlaybackState::Playing);
}


void PlaybackCompositionTest::transcodedPlaylistInsertionDoesNotReplacePlaybackQueueOrAutoPlay()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString playingPath = createAudioFile(temporaryDirectory, QStringLiteral("Playing.mp3"));
    const QString transcodedPath = temporaryDirectory.filePath(QStringLiteral("Transcoded.mp3"));
    PlaybackCompositionFixture fixture;
    PlayList playList;
    playList.add(AudioFile(playingPath, QStringLiteral("Playing")));
    fixture.playbackViewModel.replaceQueue(playList.items());
    fixture.playbackViewModel.setCurrentPlaybackIndex(0);
    QVERIFY(fixture.playbackViewModel.playCommand()->execute());
    QCOMPARE(fixture.playbackService.source(), playingPath);
    const int setSourceCountBefore = fixture.playbackService.setSourceCount();

    PlaylistCollectionUseCase playlistUseCase;
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library-v2.json"));
    const auto document = playlistUseCase.createEmptyDocument(QStringLiteral("default"),
                                                              QDateTime::fromSecsSinceEpoch(1000, Qt::UTC),
                                                              QStringLiteral("Default"))
                              .document;
    QVERIFY(playlistUseCase.replaceWithEmptyV2(storagePath, document).ok());
    TranscodedPlaylistService transcodedService(storagePath);

    QVERIFY(transcodedService.addOutput(AudioFile(transcodedPath, QStringLiteral("Transcoded")),
                                        QDateTime::fromSecsSinceEpoch(2000, Qt::UTC))
                .ok());

    QCOMPARE(fixture.playbackService.source(), playingPath);
    QCOMPARE(fixture.playbackService.setSourceCount(), setSourceCountBefore);
    QCOMPARE(fixture.playbackViewModel.currentPlaybackIndex(), 0);
    QCOMPARE(fixture.playbackViewModel.playbackState(), PlaybackState::Playing);
}

QTEST_MAIN(PlaybackCompositionTest)

#include "playback_composition_test.moc"
