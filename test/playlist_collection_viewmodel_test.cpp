#include "Model/AudioFile.h"
#include "ViewModel/PlaylistCollectionViewModel.h"
#include "ViewModel/PlaylistListModel.h"

#include <QtTest/QtTest>

#include <QSignalSpy>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::ViewModel::PlaylistCollectionViewModel;
using AudioPlayer::ViewModel::PlaylistListModel;

class PlaylistCollectionViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultStoragePathUsesStorageLibraryJson();
    void createPlaylistCommandUpdatesListModel();
    void duplicatePlaylistNameShowsError();
    void switchPlaylistCommandUpdatesVisiblePlaylistWithoutPlaybackRequest();
    void playSelectedSongCommandEmitsSnapshotAndIndex();
    void playVisiblePlaylistCommandRejectsEmptyPlaylist();
    void saveLoadCommandsRoundtripV2Document();
};

void PlaylistCollectionViewModelTest::defaultStoragePathUsesStorageLibraryJson()
{
    PlaylistCollectionViewModel viewModel;

    QCOMPARE(viewModel.storagePath(), QStringLiteral("storage/library.json"));
    QCOMPARE(viewModel.playlistCount(), 1);
    QCOMPARE(viewModel.visiblePlaylistName(), QStringLiteral("Default"));
}

void PlaylistCollectionViewModelTest::createPlaylistCommandUpdatesListModel()
{
    PlaylistCollectionViewModel viewModel;
    viewModel.setNewPlaylistName(QStringLiteral("Road Trip"));
    QSignalSpy countSpy(&viewModel, &PlaylistCollectionViewModel::playlistCountChanged);

    QVERIFY(viewModel.createPlaylistCommand()->execute());

    QCOMPARE(viewModel.playlistCount(), 2);
    QCOMPARE(countSpy.count(), 1);
    auto *model = qobject_cast<PlaylistListModel *>(viewModel.playlists());
    QVERIFY(model != nullptr);
    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->data(model->index(1, 0), PlaylistListModel::NameRole).toString(), QStringLiteral("Road Trip"));
}

void PlaylistCollectionViewModelTest::duplicatePlaylistNameShowsError()
{
    PlaylistCollectionViewModel viewModel;
    viewModel.setNewPlaylistName(QStringLiteral("Road Trip"));
    QVERIFY(viewModel.createPlaylistCommand()->execute());
    viewModel.setNewPlaylistName(QStringLiteral(" Road Trip "));

    QVERIFY(!viewModel.createPlaylistCommand()->execute());

    QVERIFY(!viewModel.lastError().isEmpty());
    QCOMPARE(viewModel.playlistCount(), 2);
}

void PlaylistCollectionViewModelTest::switchPlaylistCommandUpdatesVisiblePlaylistWithoutPlaybackRequest()
{
    PlaylistCollectionViewModel viewModel;
    viewModel.setNewPlaylistName(QStringLiteral("Road Trip"));
    QVERIFY(viewModel.createPlaylistCommand()->execute());
    viewModel.setSelectedPlaylistId(QStringLiteral("playlist-2"));
    QSignalSpy visibleSpy(&viewModel, &PlaylistCollectionViewModel::visiblePlaylistChanged);
    QSignalSpy playSpy(&viewModel, &PlaylistCollectionViewModel::playRequested);

    QVERIFY(viewModel.switchPlaylistCommand()->execute());

    QCOMPARE(viewModel.visiblePlaylistId(), QStringLiteral("playlist-2"));
    QCOMPARE(viewModel.visiblePlaylistName(), QStringLiteral("Road Trip"));
    QCOMPARE(visibleSpy.count(), 1);
    QCOMPARE(playSpy.count(), 0);
}

void PlaylistCollectionViewModelTest::playSelectedSongCommandEmitsSnapshotAndIndex()
{
    PlaylistCollectionViewModel viewModel;
    QVector<AudioFile> songs{
        AudioFile(QStringLiteral("/music/one.mp3"), QStringLiteral("One")),
        AudioFile(QStringLiteral("/music/two.mp3"), QStringLiteral("Two")),
    };
    viewModel.setCurrentVisibleSongs(songs);
    viewModel.setSelectedSongIndex(1);
    QSignalSpy playSpy(&viewModel, &PlaylistCollectionViewModel::playRequested);

    QVERIFY(viewModel.playSelectedSongCommand()->execute());

    QCOMPARE(playSpy.count(), 1);
    const QList<QVariant> arguments = playSpy.takeFirst();
    QCOMPARE(arguments.at(0).toString(), viewModel.visiblePlaylistId());
    const auto emittedSongs = qvariant_cast<QVector<AudioFile>>(arguments.at(1));
    QCOMPARE(emittedSongs.size(), 2);
    QCOMPARE(emittedSongs.at(1).title(), QStringLiteral("Two"));
    QCOMPARE(arguments.at(2).toInt(), 1);
}

void PlaylistCollectionViewModelTest::playVisiblePlaylistCommandRejectsEmptyPlaylist()
{
    PlaylistCollectionViewModel viewModel;
    QSignalSpy playSpy(&viewModel, &PlaylistCollectionViewModel::playRequested);

    QVERIFY(!viewModel.playVisiblePlaylistCommand()->execute());

    QCOMPARE(playSpy.count(), 0);
    QVERIFY(!viewModel.lastError().isEmpty());
}

void PlaylistCollectionViewModelTest::saveLoadCommandsRoundtripV2Document()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    PlaylistCollectionViewModel viewModel;
    viewModel.setStoragePath(temporaryDirectory.filePath(QStringLiteral("storage/library.json")));
    viewModel.setNewPlaylistName(QStringLiteral("Road Trip"));
    QVERIFY(viewModel.createPlaylistCommand()->execute());
    QVERIFY(viewModel.saveCommand()->execute());

    PlaylistCollectionViewModel loadedViewModel;
    loadedViewModel.setStoragePath(viewModel.storagePath());
    QVERIFY(loadedViewModel.loadCommand()->execute());

    QCOMPARE(loadedViewModel.playlistCount(), 2);
    QVERIFY(loadedViewModel.lastError().isEmpty());
}

QTEST_MAIN(PlaylistCollectionViewModelTest)
#include "playlist_collection_viewmodel_test.moc"
