#include "Model/AudioFile.h"
#include "Model/LibraryDocument.h"
#include "Model/LibraryPlaylist.h"

#include <QtTest/QtTest>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::LibraryDocument;
using AudioPlayer::Model::LibraryPlaylist;

class LibraryDocumentModelTest : public QObject
{
    Q_OBJECT

private slots:
    void emptyDocumentFactoryCreatesValidSchemaVersionTwoDocument();
    void playlistRejectsDuplicateSongPathAsNoOp();
    void playlistUpdatesTimestampWhenSongIsAdded();
    void documentRejectsDuplicatePlaylistIdAndName();
    void documentRepairsVisiblePlaylistWhenVisiblePlaylistIsDeleted();
    void documentCreatesDefaultPlaylistWhenLastPlaylistIsDeleted();
    void documentRejectsUnknownVisiblePlaylistId();
};

namespace {

QDateTime utcTime(int seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC);
}

LibraryPlaylist makePlaylist(const QString &id, const QString &name, int timestampSeconds)
{
    const QDateTime timestamp = utcTime(timestampSeconds);
    return LibraryPlaylist(id, name, timestamp, timestamp);
}

} // namespace

void LibraryDocumentModelTest::emptyDocumentFactoryCreatesValidSchemaVersionTwoDocument()
{
    const QDateTime timestamp = utcTime(1000);

    const LibraryDocument document = LibraryDocument::createEmpty(QStringLiteral("playlist-1"),
                                                                  timestamp,
                                                                  QStringLiteral("Library"));

    QCOMPARE(document.schemaVersion(), 2);
    QCOMPARE(document.playlistCount(), 1);
    QCOMPARE(document.visiblePlaylistId(), QStringLiteral("playlist-1"));
    QVERIFY(document.isValid());
    QVERIFY(document.visiblePlaylist() != nullptr);
    QCOMPARE(document.visiblePlaylist()->name(), QStringLiteral("Library"));
    QCOMPARE(document.visiblePlaylist()->createdAt(), timestamp);
    QCOMPARE(document.visiblePlaylist()->updatedAt(), timestamp);
}

void LibraryDocumentModelTest::playlistRejectsDuplicateSongPathAsNoOp()
{
    LibraryPlaylist playlist = makePlaylist(QStringLiteral("playlist-1"), QStringLiteral("Library"), 1000);
    const QDateTime firstUpdate = utcTime(1001);
    const QDateTime duplicateUpdate = utcTime(1002);
    const AudioFile firstSong(QStringLiteral("/music/song.mp3"), QStringLiteral("Song"));
    const AudioFile duplicateSong(QStringLiteral("/music/./song.mp3"), QStringLiteral("Duplicate"));

    QVERIFY(playlist.addSong(firstSong, firstUpdate));
    QVERIFY(!playlist.addSong(duplicateSong, duplicateUpdate));

    QCOMPARE(playlist.size(), 1);
    QCOMPARE(playlist.songs().first().title(), QStringLiteral("Song"));
    QCOMPARE(playlist.updatedAt(), firstUpdate);
}

void LibraryDocumentModelTest::playlistUpdatesTimestampWhenSongIsAdded()
{
    LibraryPlaylist playlist = makePlaylist(QStringLiteral("playlist-1"), QStringLiteral("Library"), 1000);
    const QDateTime updateTime = utcTime(2000);

    QVERIFY(playlist.addSong(AudioFile(QStringLiteral("/music/song.mp3"), QStringLiteral("Song")), updateTime));

    QCOMPARE(playlist.size(), 1);
    QCOMPARE(playlist.updatedAt(), updateTime);
}

void LibraryDocumentModelTest::documentRejectsDuplicatePlaylistIdAndName()
{
    LibraryDocument document = LibraryDocument::createEmpty(QStringLiteral("playlist-1"),
                                                            utcTime(1000),
                                                            QStringLiteral("Library"));

    QVERIFY(!document.addPlaylist(makePlaylist(QStringLiteral("playlist-1"), QStringLiteral("Other"), 1001)));
    QVERIFY(!document.addPlaylist(makePlaylist(QStringLiteral("playlist-2"), QStringLiteral(" Library "), 1001)));
    QVERIFY(document.addPlaylist(makePlaylist(QStringLiteral("playlist-2"), QStringLiteral("Other"), 1001)));

    QCOMPARE(document.playlistCount(), 2);
    QCOMPARE(document.visiblePlaylistId(), QStringLiteral("playlist-1"));
}

void LibraryDocumentModelTest::documentRepairsVisiblePlaylistWhenVisiblePlaylistIsDeleted()
{
    LibraryDocument document = LibraryDocument::createEmpty(QStringLiteral("playlist-1"),
                                                            utcTime(1000),
                                                            QStringLiteral("One"));
    QVERIFY(document.addPlaylist(makePlaylist(QStringLiteral("playlist-2"), QStringLiteral("Two"), 1001)));
    QVERIFY(document.addPlaylist(makePlaylist(QStringLiteral("playlist-3"), QStringLiteral("Three"), 1002)));
    QVERIFY(document.setVisiblePlaylistId(QStringLiteral("playlist-2")));

    QVERIFY(document.removePlaylistById(QStringLiteral("playlist-2"), utcTime(1003)));

    QCOMPARE(document.playlistCount(), 2);
    QCOMPARE(document.visiblePlaylistId(), QStringLiteral("playlist-3"));
    QVERIFY(document.isValid());
}

void LibraryDocumentModelTest::documentCreatesDefaultPlaylistWhenLastPlaylistIsDeleted()
{
    LibraryDocument document = LibraryDocument::createEmpty(QStringLiteral("playlist-1"),
                                                            utcTime(1000),
                                                            QStringLiteral("Only"));
    const QDateTime fallbackTimestamp = utcTime(2000);

    QVERIFY(document.removePlaylistById(QStringLiteral("playlist-1"), fallbackTimestamp));

    QCOMPARE(document.playlistCount(), 1);
    QVERIFY(document.isValid());
    QVERIFY(document.visiblePlaylist() != nullptr);
    QCOMPARE(document.visiblePlaylistId(), QStringLiteral("default"));
    QCOMPARE(document.visiblePlaylist()->name(), QStringLiteral("Default"));
    QCOMPARE(document.visiblePlaylist()->createdAt(), fallbackTimestamp);
}

void LibraryDocumentModelTest::documentRejectsUnknownVisiblePlaylistId()
{
    LibraryDocument document = LibraryDocument::createEmpty(QStringLiteral("playlist-1"),
                                                            utcTime(1000),
                                                            QStringLiteral("Library"));

    QVERIFY(!document.setVisiblePlaylistId(QStringLiteral("missing")));

    QCOMPARE(document.visiblePlaylistId(), QStringLiteral("playlist-1"));
    QVERIFY(document.isValid());
}

QTEST_MAIN(LibraryDocumentModelTest)
#include "library_document_model_test.moc"
