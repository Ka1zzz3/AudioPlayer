#include "Model/AudioFile.h"
#include "Model/JsonLibraryDocumentRepository.h"
#include "Model/LibraryDocument.h"
#include "Model/LibraryPlaylist.h"

#include <nlohmann/json.hpp>

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::JsonLibraryDocumentRepository;
using AudioPlayer::Model::LibraryDocument;
using AudioPlayer::Model::LibraryPlaylist;

class JsonLibraryDocumentRepositoryTest : public QObject
{
    Q_OBJECT

private slots:
    void defaultStoragePathUsesStorageDirectory();
    void saveCreatesParentDirectoryAndWritesSchemaVersionTwo();
    void loadReadsVersionTwoDocument();
    void saveRoundTripsDocument();
    void loadRejectsCorruptJson();
    void loadRejectsUnknownSchema();
    void loadRejectsMissingSchema();
    void loadRejectsV1FlatSongsWithoutMigration();
    void ordinarySaveRefusesToOverwriteV1FlatSongs();
    void ordinarySaveRefusesToOverwriteExternallyCorruptedFileAfterSuccessfulLoad();
    void explicitReplaceCanCreateEmptyV2OverExistingInvalidFile();
};

namespace {

QDateTime utcTime(int seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC);
}

LibraryDocument makeDocument()
{
    LibraryDocument document = LibraryDocument::createEmpty(QStringLiteral("playlist-1"),
                                                            utcTime(1000),
                                                            QStringLiteral("Library"));
    LibraryPlaylist *playlist = document.visiblePlaylist();
    Q_ASSERT(playlist != nullptr);
    playlist->addSong(AudioFile(QStringLiteral("/music/one.mp3"),
                                QStringLiteral("One"),
                                QStringLiteral("Artist"),
                                QStringLiteral("Album"),
                                123),
                      utcTime(1001));
    return document;
}

void writeTextFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    QVERIFY(QDir().mkpath(QFileInfo(path).absolutePath()));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QCOMPARE(file.write(content), static_cast<qint64>(content.size()));
}

QByteArray readFile(const QString &path)
{
    QFile file(path);
    Q_ASSERT(file.open(QIODevice::ReadOnly));
    return file.readAll();
}

} // namespace

void JsonLibraryDocumentRepositoryTest::defaultStoragePathUsesStorageDirectory()
{
    QCOMPARE(JsonLibraryDocumentRepository::defaultStoragePath(), QStringLiteral("storage/library.json"));
}

void JsonLibraryDocumentRepositoryTest::saveCreatesParentDirectoryAndWritesSchemaVersionTwo()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::SaveResult result = repository.save(makeDocument());

    QVERIFY2(result.saved, qPrintable(result.message));
    const QByteArray savedJson = readFile(storagePath);
    const nlohmann::json document = nlohmann::json::parse(savedJson.constData(),
                                                          savedJson.constData() + savedJson.size());
    QCOMPARE(document.at("schemaVersion").get<int>(), 2);
    QCOMPARE(QString::fromStdString(document.at("visiblePlaylistId").get<std::string>()),
             QStringLiteral("playlist-1"));
    QVERIFY(document.at("playlists").is_array());
    QCOMPARE(document.at("playlists").size(), 1U);
    QCOMPARE(QString::fromStdString(document.at("playlists").at(0).at("name").get<std::string>()),
             QStringLiteral("Library"));
}

void JsonLibraryDocumentRepositoryTest::loadReadsVersionTwoDocument()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath,
                  R"({
  "schemaVersion": 2,
  "visiblePlaylistId": "playlist-1",
  "playlists": [
    {
      "id": "playlist-1",
      "name": "Library",
      "createdAt": "2026-07-15T12:00:00.000Z",
      "updatedAt": "2026-07-15T12:01:00.000Z",
      "songs": [
        {
          "filePath": "/music/one.mp3",
          "title": "One",
          "artist": "Artist",
          "album": "Album",
          "durationSeconds": 123
        }
      ]
    }
  ]
})");
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::LoadResult result = repository.load();

    QVERIFY2(result.document.has_value(), qPrintable(result.message));
    QCOMPARE(result.document->schemaVersion(), 2);
    QCOMPARE(result.document->visiblePlaylistId(), QStringLiteral("playlist-1"));
    QCOMPARE(result.document->playlistCount(), 1);
    QVERIFY(result.document->visiblePlaylist() != nullptr);
    QCOMPARE(result.document->visiblePlaylist()->songs().size(), 1);
    QCOMPARE(result.document->visiblePlaylist()->songs().first().title(), QStringLiteral("One"));
}

void JsonLibraryDocumentRepositoryTest::saveRoundTripsDocument()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    JsonLibraryDocumentRepository repository(storagePath);
    const LibraryDocument document = makeDocument();

    JsonLibraryDocumentRepository::SaveResult saveResult = repository.save(document);
    QVERIFY2(saveResult.saved, qPrintable(saveResult.message));
    JsonLibraryDocumentRepository::LoadResult loadResult = repository.load();

    QVERIFY2(loadResult.document.has_value(), qPrintable(loadResult.message));
    QCOMPARE(loadResult.document->visiblePlaylistId(), document.visiblePlaylistId());
    QCOMPARE(loadResult.document->visiblePlaylist()->songs().size(), 1);
    QCOMPARE(loadResult.document->visiblePlaylist()->songs().first().filePath(), QStringLiteral("/music/one.mp3"));
}

void JsonLibraryDocumentRepositoryTest::loadRejectsCorruptJson()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath, "{ not json");
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::LoadResult result = repository.load();

    QVERIFY(!result.document.has_value());
    QCOMPARE(result.error, JsonLibraryDocumentRepository::Error::ParseFailed);
    QVERIFY(!result.message.isEmpty());
}

void JsonLibraryDocumentRepositoryTest::loadRejectsUnknownSchema()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath, R"({"schemaVersion": 99, "playlists": []})");
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::LoadResult result = repository.load();

    QVERIFY(!result.document.has_value());
    QCOMPARE(result.error, JsonLibraryDocumentRepository::Error::InvalidSchema);
}

void JsonLibraryDocumentRepositoryTest::loadRejectsMissingSchema()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath, R"({"visiblePlaylistId": "playlist-1", "playlists": []})");
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::LoadResult result = repository.load();

    QVERIFY(!result.document.has_value());
    QCOMPARE(result.error, JsonLibraryDocumentRepository::Error::InvalidSchema);
}

void JsonLibraryDocumentRepositoryTest::loadRejectsV1FlatSongsWithoutMigration()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath,
                  R"({"schemaVersion": 1, "songs": [{"filePath": "/music/old.mp3", "title": "Old"}]})");
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::LoadResult result = repository.load();

    QVERIFY(!result.document.has_value());
    QCOMPARE(result.error, JsonLibraryDocumentRepository::Error::InvalidSchema);
}

void JsonLibraryDocumentRepositoryTest::ordinarySaveRefusesToOverwriteV1FlatSongs()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    const QByteArray oldContent = R"({"schemaVersion": 1, "songs": []})";
    writeTextFile(storagePath, oldContent);
    JsonLibraryDocumentRepository repository(storagePath);

    const JsonLibraryDocumentRepository::SaveResult result = repository.save(makeDocument());

    QVERIFY(!result.saved);
    QCOMPARE(result.error, JsonLibraryDocumentRepository::Error::InvalidSchema);
    QCOMPARE(readFile(storagePath), oldContent);
}

void JsonLibraryDocumentRepositoryTest::ordinarySaveRefusesToOverwriteExternallyCorruptedFileAfterSuccessfulLoad()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    JsonLibraryDocumentRepository repository(storagePath);
    QVERIFY(repository.save(makeDocument()).saved);
    QVERIFY(repository.load().document.has_value());
    const QByteArray corruptContent = "{ corrupt after load";
    writeTextFile(storagePath, corruptContent);

    const JsonLibraryDocumentRepository::SaveResult result = repository.save(makeDocument());

    QVERIFY(!result.saved);
    QCOMPARE(result.error, JsonLibraryDocumentRepository::Error::ParseFailed);
    QCOMPARE(readFile(storagePath), corruptContent);
}

void JsonLibraryDocumentRepositoryTest::explicitReplaceCanCreateEmptyV2OverExistingInvalidFile()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("storage/library.json"));
    writeTextFile(storagePath, R"({"schemaVersion": 1, "songs": []})");
    JsonLibraryDocumentRepository repository(storagePath);
    const LibraryDocument replacement = LibraryDocument::createEmpty(QStringLiteral("playlist-new"),
                                                                      utcTime(2000),
                                                                      QStringLiteral("Default"));

    const JsonLibraryDocumentRepository::SaveResult result = repository.replaceWithEmptyV2(replacement);
    const JsonLibraryDocumentRepository::LoadResult loaded = repository.load();

    QVERIFY2(result.saved, qPrintable(result.message));
    QVERIFY2(loaded.document.has_value(), qPrintable(loaded.message));
    QCOMPARE(loaded.document->visiblePlaylistId(), QStringLiteral("playlist-new"));
}

QTEST_MAIN(JsonLibraryDocumentRepositoryTest)
#include "json_library_document_repository_test.moc"
