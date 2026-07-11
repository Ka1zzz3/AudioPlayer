#include "Model/AudioFile.h"
#include "Model/JsonSongRepository.h"
#include "Model/PlayList.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/SongListModel.h"

#include <QtTest/QtTest>

#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::JsonSongRepository;
using AudioPlayer::Model::PlayList;
using AudioPlayer::ViewModel::LibraryViewModel;
using AudioPlayer::ViewModel::SongListModel;

class ViewModelBehaviorTest : public QObject
{
    Q_OBJECT

private slots:
    void successfulLoadClearsErrorsAndUpdatesCountAndRoles();
    void failedLoadSetsLastError();
    void refreshSkipsMissingFileAndReportsError();
    void successfulRefreshClearsStaleErrors();
    void songListModelRolesExposeExpectedValues();
};

namespace {

QString createAudioFile(const QTemporaryDir &temporaryDirectory, const QString &fileName)
{
    const QString filePath = temporaryDirectory.filePath(fileName);
    QFile file(filePath);
    const bool opened = file.open(QIODevice::WriteOnly);
    Q_ASSERT(opened);
    Q_UNUSED(opened);
    const qint64 written = file.write("fake audio bytes");
    Q_ASSERT(written > 0);
    Q_UNUSED(written);
    file.close();
    return QFileInfo(filePath).absoluteFilePath();
}

QString savePlayList(const QTemporaryDir &temporaryDirectory, const PlayList &playList)
{
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    JsonSongRepository repository(storagePath);
    QString errorMessage;
    const bool saved = repository.save(playList, &errorMessage);
    Q_ASSERT(saved);
    Q_UNUSED(saved);
    Q_ASSERT(errorMessage.isEmpty());
    return storagePath;
}

} // namespace

void ViewModelBehaviorTest::successfulLoadClearsErrorsAndUpdatesCountAndRoles()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString audioPath = createAudioFile(temporaryDirectory, QStringLiteral("Loaded Track.mp3"));
    PlayList playList;
    playList.add(AudioFile(audioPath,
                           QStringLiteral("Stored Title"),
                           QStringLiteral("Stored Artist"),
                           QStringLiteral("Stored Album"),
                           99));
    const QString storagePath = savePlayList(temporaryDirectory, playList);

    LibraryViewModel viewModel;
    viewModel.setStoragePath(temporaryDirectory.filePath(QStringLiteral("missing.json")));
    QVERIFY(!viewModel.load());
    QVERIFY(!viewModel.lastError().isEmpty());

    QSignalSpy loadedSpy(&viewModel, &LibraryViewModel::loaded);
    QSignalSpy countSpy(&viewModel, &LibraryViewModel::countChanged);
    viewModel.setStoragePath(storagePath);

    QVERIFY(viewModel.load());
    QCOMPARE(viewModel.lastError(), QString());
    QCOMPARE(viewModel.count(), 1);
    QCOMPARE(viewModel.songs()->rowCount(), 1);
    QCOMPARE(loadedSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);

    const QModelIndex index = viewModel.songs()->index(0, 0);
    QCOMPARE(viewModel.songs()->data(index, SongListModel::FilePathRole).toString(), audioPath);
    QCOMPARE(viewModel.songs()->data(index, SongListModel::TitleRole).toString(), QStringLiteral("Stored Title"));
    QCOMPARE(viewModel.songs()->data(index, SongListModel::ArtistRole).toString(), QStringLiteral("Stored Artist"));
    QCOMPARE(viewModel.songs()->data(index, SongListModel::AlbumRole).toString(), QStringLiteral("Stored Album"));
    QCOMPARE(viewModel.songs()->data(index, SongListModel::DurationSecondsRole).toLongLong(), 99);
    QCOMPARE(viewModel.songs()->data(index, SongListModel::DisplayTitleRole).toString(), QStringLiteral("Stored Title"));
}

void ViewModelBehaviorTest::failedLoadSetsLastError()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    LibraryViewModel viewModel;
    viewModel.setStoragePath(temporaryDirectory.filePath(QStringLiteral("missing.json")));

    QSignalSpy failedSpy(&viewModel, &LibraryViewModel::loadFailed);

    QVERIFY(!viewModel.load());
    QVERIFY(!viewModel.lastError().isEmpty());
    QVERIFY(viewModel.lastError().contains(QStringLiteral("reading"), Qt::CaseInsensitive));
    QCOMPARE(failedSpy.count(), 1);
    QCOMPARE(failedSpy.takeFirst().at(0).toString(), viewModel.lastError());
}

void ViewModelBehaviorTest::refreshSkipsMissingFileAndReportsError()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString keepPath = createAudioFile(temporaryDirectory, QStringLiteral("Keep.wav"));
    const QString missingPath = temporaryDirectory.filePath(QStringLiteral("Missing.mp3"));
    PlayList playList;
    playList.add(AudioFile(keepPath, QStringLiteral("Old Keep")));
    playList.add(AudioFile(missingPath, QStringLiteral("Missing")));
    const QString storagePath = savePlayList(temporaryDirectory, playList);

    LibraryViewModel viewModel;
    viewModel.setStoragePath(storagePath);
    QVERIFY(viewModel.load());

    QSignalSpy refreshedSpy(&viewModel, &LibraryViewModel::refreshed);
    QSignalSpy countSpy(&viewModel, &LibraryViewModel::countChanged);

    QVERIFY(viewModel.refresh());
    QCOMPARE(viewModel.count(), 1);
    QCOMPARE(viewModel.songs()->rowCount(), 1);
    QVERIFY(!viewModel.lastError().isEmpty());
    QVERIFY(viewModel.lastError().contains(QStringLiteral("1")));
    QVERIFY(viewModel.lastError().contains(missingPath));
    QCOMPARE(viewModel.songs()->data(viewModel.songs()->index(0, 0), SongListModel::FilePathRole).toString(), keepPath);
    QCOMPARE(viewModel.songs()->data(viewModel.songs()->index(0, 0), SongListModel::TitleRole).toString(), QStringLiteral("Keep"));
    QCOMPARE(refreshedSpy.count(), 1);
    QCOMPARE(countSpy.count(), 1);
}

void ViewModelBehaviorTest::successfulRefreshClearsStaleErrors()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString audioPath = createAudioFile(temporaryDirectory, QStringLiteral("Fresh.flac"));
    PlayList playList;
    playList.add(AudioFile(audioPath, QStringLiteral("Old Fresh")));
    const QString storagePath = savePlayList(temporaryDirectory, playList);

    LibraryViewModel viewModel;
    viewModel.setStoragePath(storagePath);
    QVERIFY(viewModel.load());
    viewModel.setStoragePath(temporaryDirectory.filePath(QStringLiteral("missing.json")));
    QVERIFY(!viewModel.load());
    QVERIFY(!viewModel.lastError().isEmpty());

    QVERIFY(viewModel.refresh());
    QCOMPARE(viewModel.lastError(), QString());
    QCOMPARE(viewModel.count(), 1);
    QCOMPARE(viewModel.songs()->data(viewModel.songs()->index(0, 0), SongListModel::TitleRole).toString(), QStringLiteral("Fresh"));
}

void ViewModelBehaviorTest::songListModelRolesExposeExpectedValues()
{
    SongListModel model;
    PlayList playList;
    playList.add(AudioFile(QStringLiteral("/music/song.mp3"),
                           QStringLiteral("Title"),
                           QStringLiteral("Artist"),
                           QStringLiteral("Album"),
                           321));

    QSignalSpy resetSpy(&model, &SongListModel::modelReset);
    model.setPlayList(std::move(playList));

    QCOMPARE(resetSpy.count(), 1);
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.rowCount(model.index(0, 0)), 0);
    const QModelIndex index = model.index(0, 0);
    QCOMPARE(model.data(index, SongListModel::FilePathRole).toString(), QStringLiteral("/music/song.mp3"));
    QCOMPARE(model.data(index, SongListModel::TitleRole).toString(), QStringLiteral("Title"));
    QCOMPARE(model.data(index, SongListModel::ArtistRole).toString(), QStringLiteral("Artist"));
    QCOMPARE(model.data(index, SongListModel::AlbumRole).toString(), QStringLiteral("Album"));
    QCOMPARE(model.data(index, SongListModel::DurationSecondsRole).toLongLong(), 321);
    QCOMPARE(model.data(index, SongListModel::DisplayTitleRole).toString(), QStringLiteral("Title"));
    QCOMPARE(model.data(index, Qt::DisplayRole).toString(), QStringLiteral("Title"));

    const QHash<int, QByteArray> roles = model.roleNames();
    QCOMPARE(roles.value(SongListModel::FilePathRole), QByteArray("filePath"));
    QCOMPARE(roles.value(SongListModel::TitleRole), QByteArray("title"));
    QCOMPARE(roles.value(SongListModel::ArtistRole), QByteArray("artist"));
    QCOMPARE(roles.value(SongListModel::AlbumRole), QByteArray("album"));
    QCOMPARE(roles.value(SongListModel::DurationSecondsRole), QByteArray("durationSeconds"));
    QCOMPARE(roles.value(SongListModel::DisplayTitleRole), QByteArray("displayTitle"));
    QVERIFY(!model.data(QModelIndex(), SongListModel::TitleRole).isValid());
}

QTEST_MAIN(ViewModelBehaviorTest)

#include "viewmodel_behavior_test.moc"
