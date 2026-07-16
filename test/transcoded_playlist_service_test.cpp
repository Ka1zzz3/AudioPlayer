#include "Model/Service/TranscodedPlaylistService.h"

#include "Model/JsonLibraryDocumentRepository.h"

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::JsonLibraryDocumentRepository;
using AudioPlayer::Model::LibraryDocument;
using AudioPlayer::Model::Service::PlaylistCollectionUseCase;
using AudioPlayer::Model::Service::TranscodedPlaylistService;

class TranscodedPlaylistServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void addOutputCreatesTranscodedPlaylistAndPersistsV2Document();
    void addOutputReusesTranscodedPlaylistWithoutDuplicatingSong();
    void addOutputReportsSavePreflightFailureForCorruptStorage();
};

namespace {

QDateTime utcTime(int seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC);
}

QString createInitialStorage(QTemporaryDir &temporaryDir)
{
    const QString storagePath = temporaryDir.filePath(QStringLiteral("library.json"));
    PlaylistCollectionUseCase useCase;
    const auto document = useCase.createEmptyDocument(QStringLiteral("default"), utcTime(1000), QStringLiteral("Default")).document;
    const auto saved = useCase.replaceWithEmptyV2(storagePath, document);
    Q_ASSERT(saved.ok());
    return storagePath;
}

} // namespace

void TranscodedPlaylistServiceTest::addOutputCreatesTranscodedPlaylistAndPersistsV2Document()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString storagePath = createInitialStorage(temporaryDir);
    TranscodedPlaylistService service(storagePath);

    const auto result = service.addOutput(AudioFile(QStringLiteral("/out/song.mp3"), QStringLiteral("Song")), utcTime(2000));

    QVERIFY(result.ok());
    QVERIFY(result.addedToPlaylist);

    JsonLibraryDocumentRepository repository(storagePath);
    const auto loaded = repository.load();
    QVERIFY(loaded.document.has_value());
    const LibraryDocument &document = *loaded.document;
    const int transcodedIndex = document.findPlaylistById(QStringLiteral("transcoded"));
    QVERIFY(transcodedIndex >= 0);
    const auto *playlist = document.playlistAt(transcodedIndex);
    QVERIFY(playlist != nullptr);
    QCOMPARE(playlist->name(), QStringLiteral("Transcoded"));
    QCOMPARE(playlist->size(), 1);
    QCOMPARE(playlist->songs().first().filePath(), QStringLiteral("/out/song.mp3"));
}

void TranscodedPlaylistServiceTest::addOutputReusesTranscodedPlaylistWithoutDuplicatingSong()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString storagePath = createInitialStorage(temporaryDir);
    TranscodedPlaylistService service(storagePath);
    const AudioFile song(QStringLiteral("/out/song.mp3"), QStringLiteral("Song"));

    QVERIFY(service.addOutput(song, utcTime(2000)).ok());
    QVERIFY(service.addOutput(song, utcTime(2001)).ok());

    JsonLibraryDocumentRepository repository(storagePath);
    const auto loaded = repository.load();
    QVERIFY(loaded.document.has_value());
    const auto *playlist = loaded.document->playlistAt(loaded.document->findPlaylistById(QStringLiteral("transcoded")));
    QVERIFY(playlist != nullptr);
    QCOMPARE(playlist->size(), 1);
}

void TranscodedPlaylistServiceTest::addOutputReportsSavePreflightFailureForCorruptStorage()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString storagePath = temporaryDir.filePath(QStringLiteral("library.json"));
    QFile file(storagePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("not json");
    file.close();
    TranscodedPlaylistService service(storagePath);

    const auto result = service.addOutput(AudioFile(QStringLiteral("/out/song.mp3"), QStringLiteral("Song")), utcTime(2000));

    QVERIFY(!result.ok());
    QVERIFY(!result.error.isEmpty());
}

QTEST_MAIN(TranscodedPlaylistServiceTest)
#include "transcoded_playlist_service_test.moc"
