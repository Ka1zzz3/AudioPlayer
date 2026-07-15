#include "Model/AudioFile.h"
#include "Model/JsonSongRepository.h"
#include "Model/PlayList.h"
#include "Model/Service/LibraryUseCase.h"

#include <QtTest/QtTest>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::JsonSongRepository;
using AudioPlayer::Model::PlayList;
using AudioPlayer::Model::Service::LibraryUseCase;
using AudioPlayer::Model::Service::LibraryWorkflowResult;

class LibraryUseCaseTest : public QObject
{
    Q_OBJECT

private slots:
    void loadSuccessReturnsStoredPlaylist();
    void loadFailureReturnsRepositoryError();
    void scanDirectoryReturnsSupportedFilesAndWarnings();
    void refreshSkipsMissingOrUnsupportedFiles();
    void saveLoadRoundtripPreservesJsonRepositoryBehavior();
};

namespace {

QString createFile(const QString &filePath, const QByteArray &contents = QByteArray("fake audio bytes"))
{
    QFile file(filePath);
    const bool opened = file.open(QIODevice::WriteOnly);
    Q_ASSERT(opened);
    Q_UNUSED(opened);
    const qint64 written = file.write(contents);
    Q_ASSERT(written > 0);
    Q_UNUSED(written);
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
    Q_UNUSED(saved);
    Q_ASSERT(errorMessage.isEmpty());
    return storagePath;
}

} // namespace

void LibraryUseCaseTest::loadSuccessReturnsStoredPlaylist()
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

    const LibraryWorkflowResult result = LibraryUseCase().load(storagePath);

    QVERIFY2(result.ok(), qPrintable(result.error));
    QVERIFY(result.warnings.isEmpty());
    QCOMPARE(result.playList.size(), 1);
    QVERIFY(result.playList.at(0) != nullptr);
    QCOMPARE(*result.playList.at(0), *playList.at(0));
}

void LibraryUseCaseTest::loadFailureReturnsRepositoryError()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const LibraryWorkflowResult result = LibraryUseCase().load(temporaryDirectory.filePath(QStringLiteral("missing.json")));

    QVERIFY(!result.ok());
    QVERIFY(!result.error.isEmpty());
    QVERIFY(result.error.contains(QStringLiteral("reading"), Qt::CaseInsensitive));
    QCOMPARE(result.playList.size(), 0);
    QVERIFY(result.warnings.isEmpty());
}

void LibraryUseCaseTest::scanDirectoryReturnsSupportedFilesAndWarnings()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString scanPath = temporaryDirectory.filePath(QStringLiteral("scan"));
    QVERIFY(QDir().mkpath(scanPath));
    const QString audioPath = createFile(QDir(scanPath).filePath(QStringLiteral("Scanned Track.MP3")));
    const QString unsupportedPath = createFile(QDir(scanPath).filePath(QStringLiteral("notes.txt")), QByteArray("not audio"));

    const LibraryWorkflowResult result = LibraryUseCase().scanDirectory(scanPath);

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.playList.size(), 1);
    QVERIFY(result.playList.at(0) != nullptr);
    QCOMPARE(result.playList.at(0)->filePath(), audioPath);
    QCOMPARE(result.playList.at(0)->title(), QStringLiteral("Scanned Track"));
    QCOMPARE(result.playList.at(0)->artist(), QStringLiteral("Unknown Artist"));
    QCOMPARE(result.playList.at(0)->album(), QStringLiteral("Unknown Album"));
    QCOMPARE(result.playList.at(0)->durationSeconds(), 0);
    QCOMPARE(result.warnings.size(), 1);
    QVERIFY(result.warnings.at(0).contains(unsupportedPath));
}

void LibraryUseCaseTest::refreshSkipsMissingOrUnsupportedFiles()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString keepPath = createAudioFile(temporaryDirectory, QStringLiteral("Keep.wav"));
    const QString missingPath = temporaryDirectory.filePath(QStringLiteral("Missing.mp3"));
    const QString unsupportedPath = createFile(temporaryDirectory.filePath(QStringLiteral("notes.txt")), QByteArray("not audio"));
    PlayList playList;
    playList.add(AudioFile(keepPath, QStringLiteral("Old Keep")));
    playList.add(AudioFile(missingPath, QStringLiteral("Missing")));
    playList.add(AudioFile(unsupportedPath, QStringLiteral("Unsupported")));

    const LibraryWorkflowResult result = LibraryUseCase().refresh(playList);

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.playList.size(), 1);
    QVERIFY(result.playList.at(0) != nullptr);
    QCOMPARE(result.playList.at(0)->filePath(), keepPath);
    QCOMPARE(result.playList.at(0)->title(), QStringLiteral("Keep"));
    QCOMPARE(result.warnings.size(), 2);
    QCOMPARE(result.warnings.at(0), missingPath);
    QCOMPARE(result.warnings.at(1), unsupportedPath);
}

void LibraryUseCaseTest::saveLoadRoundtripPreservesJsonRepositoryBehavior()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("nested/library.json"));
    PlayList playList;
    playList.add(AudioFile(QStringLiteral("/music/one.mp3"),
                           QStringLiteral("One"),
                           QStringLiteral("Artist"),
                           QStringLiteral("Album"),
                           123));
    playList.add(AudioFile(QStringLiteral("/music/two.flac"), QStringLiteral("Two"), {}, {}, 456));

    const LibraryUseCase useCase;
    const LibraryWorkflowResult saveResult = useCase.save(storagePath, playList);
    QVERIFY2(saveResult.ok(), qPrintable(saveResult.error));

    const LibraryWorkflowResult loadResult = useCase.load(storagePath);

    QVERIFY2(loadResult.ok(), qPrintable(loadResult.error));
    QCOMPARE(loadResult.playList.size(), 2);
    QCOMPARE(*loadResult.playList.at(0), *playList.at(0));
    QCOMPARE(*loadResult.playList.at(1), *playList.at(1));
}

QTEST_MAIN(LibraryUseCaseTest)

#include "library_usecase_test.moc"
