#include "Model/AudioFile.h"
#include "Model/JsonLibraryDocumentRepository.h"
#include "Model/LibraryDocument.h"
#include "Model/Service/PlaylistCollectionUseCase.h"

#include <QtTest/QtTest>

#include <QFile>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::JsonLibraryDocumentRepository;
using AudioPlayer::Model::LibraryDocument;
using AudioPlayer::Model::Service::PlaylistCollectionResult;
using AudioPlayer::Model::Service::PlaylistCollectionUseCase;

class PlaylistCollectionUseCaseTest : public QObject
{
    Q_OBJECT

private slots:
    void createEmptyDocumentCreatesDefaultVisiblePlaylist();
    void createPlaylistRejectsDuplicateName();
    void switchVisiblePlaylistUpdatesVisibleId();
    void deleteVisiblePlaylistSelectsNeighbor();
    void deleteLastPlaylistCreatesDefaultPlaylist();
    void addSongToPlaylistAddsSongAndUpdatesTimestamp();
    void addDuplicateSongIsVisibleNoOpWithStatus();
    void saveAndLoadRoundtripUsesV2Repository();
    void ordinarySaveRejectsExistingV1WithoutMigration();
    void replaceWithEmptyV2IsExplicitlySeparateFromOrdinarySave();
};

namespace {

QDateTime utcTime(int seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC);
}

LibraryDocument makeDocument()
{
    PlaylistCollectionResult result = PlaylistCollectionUseCase().createEmptyDocument(QStringLiteral("playlist-1"),
                                                                                      utcTime(1000),
                                                                                      QStringLiteral("One"));
    Q_ASSERT(result.ok());
    return result.document;
}

void writeTextFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(content), static_cast<qint64>(content.size()));
}

} // namespace

void PlaylistCollectionUseCaseTest::createEmptyDocumentCreatesDefaultVisiblePlaylist()
{
    const PlaylistCollectionResult result = PlaylistCollectionUseCase().createEmptyDocument(QStringLiteral("playlist-1"),
                                                                                            utcTime(1000));

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.document.schemaVersion(), 2);
    QCOMPARE(result.document.playlistCount(), 1);
    QCOMPARE(result.document.visiblePlaylistId(), QStringLiteral("playlist-1"));
    QVERIFY(result.document.isValid());
}

void PlaylistCollectionUseCaseTest::createPlaylistRejectsDuplicateName()
{
    const LibraryDocument document = makeDocument();

    const PlaylistCollectionResult duplicateName = PlaylistCollectionUseCase().createPlaylist(document,
                                                                                              QStringLiteral("playlist-2"),
                                                                                              QStringLiteral(" One "),
                                                                                              utcTime(1001));

    QVERIFY(!duplicateName.ok());
    QVERIFY(!duplicateName.error.isEmpty());
    QCOMPARE(duplicateName.document.playlistCount(), 1);
}

void PlaylistCollectionUseCaseTest::switchVisiblePlaylistUpdatesVisibleId()
{
    const PlaylistCollectionUseCase useCase;
    PlaylistCollectionResult created = useCase.createPlaylist(makeDocument(),
                                                              QStringLiteral("playlist-2"),
                                                              QStringLiteral("Two"),
                                                              utcTime(1001));
    QVERIFY2(created.ok(), qPrintable(created.error));

    const PlaylistCollectionResult switched = useCase.switchVisiblePlaylist(created.document,
                                                                            QStringLiteral("playlist-2"));

    QVERIFY2(switched.ok(), qPrintable(switched.error));
    QCOMPARE(switched.document.visiblePlaylistId(), QStringLiteral("playlist-2"));
}

void PlaylistCollectionUseCaseTest::deleteVisiblePlaylistSelectsNeighbor()
{
    const PlaylistCollectionUseCase useCase;
    PlaylistCollectionResult result = useCase.createPlaylist(makeDocument(),
                                                             QStringLiteral("playlist-2"),
                                                             QStringLiteral("Two"),
                                                             utcTime(1001));
    QVERIFY(result.ok());
    result = useCase.createPlaylist(result.document,
                                    QStringLiteral("playlist-3"),
                                    QStringLiteral("Three"),
                                    utcTime(1002));
    QVERIFY(result.ok());
    result = useCase.switchVisiblePlaylist(result.document, QStringLiteral("playlist-2"));
    QVERIFY(result.ok());

    const PlaylistCollectionResult deleted = useCase.deletePlaylist(result.document,
                                                                    QStringLiteral("playlist-2"),
                                                                    utcTime(1003));

    QVERIFY2(deleted.ok(), qPrintable(deleted.error));
    QCOMPARE(deleted.document.playlistCount(), 2);
    QCOMPARE(deleted.document.visiblePlaylistId(), QStringLiteral("playlist-3"));
}

void PlaylistCollectionUseCaseTest::deleteLastPlaylistCreatesDefaultPlaylist()
{
    const PlaylistCollectionResult deleted = PlaylistCollectionUseCase().deletePlaylist(makeDocument(),
                                                                                        QStringLiteral("playlist-1"),
                                                                                        utcTime(2000));

    QVERIFY2(deleted.ok(), qPrintable(deleted.error));
    QCOMPARE(deleted.document.playlistCount(), 1);
    QCOMPARE(deleted.document.visiblePlaylistId(), QStringLiteral("default"));
    QVERIFY(deleted.document.visiblePlaylist() != nullptr);
    QCOMPARE(deleted.document.visiblePlaylist()->name(), QStringLiteral("Default"));
    QCOMPARE(deleted.document.visiblePlaylist()->createdAt(), utcTime(2000));
}

void PlaylistCollectionUseCaseTest::addSongToPlaylistAddsSongAndUpdatesTimestamp()
{
    const AudioFile song(QStringLiteral("/music/one.mp3"), QStringLiteral("One"));

    const PlaylistCollectionResult result = PlaylistCollectionUseCase().addSongToPlaylist(makeDocument(),
                                                                                          QStringLiteral("playlist-1"),
                                                                                          song,
                                                                                          utcTime(3000));

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.document.visiblePlaylist()->songs().size(), 1);
    QCOMPARE(result.document.visiblePlaylist()->songs().first(), song);
    QCOMPARE(result.document.visiblePlaylist()->updatedAt(), utcTime(3000));
}

void PlaylistCollectionUseCaseTest::addDuplicateSongIsVisibleNoOpWithStatus()
{
    const PlaylistCollectionUseCase useCase;
    const AudioFile song(QStringLiteral("/music/one.mp3"), QStringLiteral("One"));
    PlaylistCollectionResult result = useCase.addSongToPlaylist(makeDocument(),
                                                                QStringLiteral("playlist-1"),
                                                                song,
                                                                utcTime(3000));
    QVERIFY(result.ok());

    const PlaylistCollectionResult duplicate = useCase.addSongToPlaylist(result.document,
                                                                         QStringLiteral("playlist-1"),
                                                                         AudioFile(QStringLiteral("/music/./one.mp3"),
                                                                                   QStringLiteral("Duplicate")),
                                                                         utcTime(4000));

    QVERIFY2(duplicate.ok(), qPrintable(duplicate.error));
    QVERIFY(duplicate.status.contains(QStringLiteral("already exists"), Qt::CaseInsensitive));
    QCOMPARE(duplicate.document.visiblePlaylist()->songs().size(), 1);
    QCOMPARE(duplicate.document.visiblePlaylist()->updatedAt(), utcTime(3000));
}

void PlaylistCollectionUseCaseTest::saveAndLoadRoundtripUsesV2Repository()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    const PlaylistCollectionUseCase useCase;
    const LibraryDocument document = makeDocument();

    const PlaylistCollectionResult saved = useCase.save(storagePath, document);
    const PlaylistCollectionResult loaded = useCase.load(storagePath);

    QVERIFY2(saved.ok(), qPrintable(saved.error));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    QCOMPARE(loaded.document.schemaVersion(), 2);
    QCOMPARE(loaded.document.visiblePlaylistId(), document.visiblePlaylistId());
}

void PlaylistCollectionUseCaseTest::ordinarySaveRejectsExistingV1WithoutMigration()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath, R"({"schemaVersion": 1, "songs": []})");

    const PlaylistCollectionResult saved = PlaylistCollectionUseCase().save(storagePath, makeDocument());

    QVERIFY(!saved.ok());
    QVERIFY(saved.error.contains(QStringLiteral("Refusing to overwrite"), Qt::CaseInsensitive));
}

void PlaylistCollectionUseCaseTest::replaceWithEmptyV2IsExplicitlySeparateFromOrdinarySave()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath, R"({"schemaVersion": 1, "songs": []})");
    const PlaylistCollectionUseCase useCase;
    const LibraryDocument replacement = LibraryDocument::createEmpty(QStringLiteral("playlist-new"),
                                                                      utcTime(5000));

    const PlaylistCollectionResult replaced = useCase.replaceWithEmptyV2(storagePath, replacement);
    const PlaylistCollectionResult loaded = useCase.load(storagePath);

    QVERIFY2(replaced.ok(), qPrintable(replaced.error));
    QVERIFY2(loaded.ok(), qPrintable(loaded.error));
    QCOMPARE(loaded.document.visiblePlaylistId(), QStringLiteral("playlist-new"));
}

QTEST_MAIN(PlaylistCollectionUseCaseTest)
#include "playlist_collection_usecase_test.moc"
