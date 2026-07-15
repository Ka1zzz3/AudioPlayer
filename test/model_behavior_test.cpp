#include "Model/AudioFile.h"
#include "Model/FileScanner.h"
#include "Model/JsonSongRepository.h"
#include "Model/PlayList.h"

#include <nlohmann/json.hpp>

#include <QtTest/QtTest>

#include <optional>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

using AudioPlayer::Model::AudioFile;
using AudioPlayer::Model::FileScanner;
using AudioPlayer::Model::JsonSongRepository;
using AudioPlayer::Model::PlayList;
using AudioPlayer::Model::ScanResult;

class ModelBehaviorTest : public QObject
{
    Q_OBJECT

private slots:
    void audioFileClampsNegativeDurationToZero();
    void audioFileUsesTrimmedTitleBeforePathForDisplayTitle();
    void playListTracksCurrentFileAndTotalDuration();
    void jsonSongRepositorySavesAndLoadsPlaylist();
    void jsonSongRepositorySaveEmitsCurrentLibraryJsonSchema();
    void jsonSongRepositoryEmptyStoragePathFailsWithError();
    void jsonSongRepositoryCorruptJsonLoadFails();
    void jsonSongRepositoryJsonRootNotObjectLoadFails();
    void jsonSongRepositoryMissingSongsArrayLoadFails();
    void jsonSongRepositorySongEntryMissingFilePathLoadFails();
    void jsonSongRepositorySongEntryEmptyFilePathLoadFails();
    void jsonSongRepositoryNonIntegerDurationLoadFails();
    void jsonSongRepositorySaveInvalidAudioFileFails();
    void jsonSongRepositoryScanDirectorySaveLoadIntegration();
    void fileScannerRecognizesSupportedExtensionsCaseInsensitively();
    void fileScannerRejectsMissingOrUnsupportedFiles();
    void fileScannerCreatesAudioFileForSupportedFile();
    void audioFileExtensionLowercasesAndStripsDot();
    void fileScannerScanDirectoryReturnsSupportedFilesAndWarnings();
    void fileScannerScanDirectoryRecursiveFlagControlsSubdirectoryScanning();
    void fileScannerScanDirectoryHandlesExtensionsCaseInsensitively();
    void fileScannerScanDirectoryMissingDirectoryIsFatal();
    void fileScannerScanDirectoryFilePathIsFatal();
    void fileScannerScanDirectoryEmptyDirectoryIsOk();
    void fileScannerScanDirectoryOrdersByAbsolutePath();
    void fileScannerScanDirectoryUnreadableDirectoryIsFatal();
};

void ModelBehaviorTest::audioFileClampsNegativeDurationToZero()
{
    AudioFile audioFile(QStringLiteral("/music/song.mp3"));

    audioFile.setDurationSeconds(-42);

    QCOMPARE(audioFile.durationSeconds(), 0);
}

void ModelBehaviorTest::audioFileUsesTrimmedTitleBeforePathForDisplayTitle()
{
    AudioFile audioFile(QStringLiteral("/music/song.mp3"));

    audioFile.setTitle(QStringLiteral("  Song Title  "));

    QCOMPARE(audioFile.displayTitle(), QStringLiteral("Song Title"));
}

void ModelBehaviorTest::playListTracksCurrentFileAndTotalDuration()
{
    PlayList playList;

    playList.add(AudioFile(QStringLiteral("/music/one.mp3"), QStringLiteral("One"), {}, {}, 10));
    playList.add(AudioFile(QStringLiteral("/music/two.wav"), QStringLiteral("Two"), {}, {}, 20));
    QVERIFY(playList.setCurrentIndex(1));

    QCOMPARE(playList.size(), 2);
    QCOMPARE(playList.currentIndex(), 1);
    QVERIFY(playList.currentFile().has_value());
    QCOMPARE(playList.currentFile()->filePath(), QStringLiteral("/music/two.wav"));
    QCOMPARE(playList.totalDurationSeconds(), 30);
}

void ModelBehaviorTest::jsonSongRepositorySavesAndLoadsPlaylist()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("nested/library.json"));
    JsonSongRepository repository(storagePath);
    PlayList playList;
    playList.add(AudioFile(QStringLiteral("/music/one.mp3"),
                           QStringLiteral("One"),
                           QStringLiteral("Artist"),
                           QStringLiteral("Album"),
                           123));
    playList.add(AudioFile(QStringLiteral("/music/two.flac"), QStringLiteral("Two"), {}, {}, 456));

    QString errorMessage;
    QVERIFY2(repository.save(playList, &errorMessage), qPrintable(errorMessage));
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY2(loaded.has_value(), qPrintable(errorMessage));
    QCOMPARE(loaded->size(), 2);
    QCOMPARE(*loaded->at(0), *playList.at(0));
    QCOMPARE(*loaded->at(1), *playList.at(1));
}

void ModelBehaviorTest::jsonSongRepositorySaveEmitsCurrentLibraryJsonSchema()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    JsonSongRepository repository(storagePath);
    PlayList playList;
    playList.add(AudioFile(QStringLiteral("/music/one.mp3"),
                           QStringLiteral("One"),
                           QStringLiteral("Artist"),
                           QStringLiteral("Album"),
                           123));

    QString errorMessage;
    QVERIFY2(repository.save(playList, &errorMessage), qPrintable(errorMessage));

    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::ReadOnly));
    const QByteArray savedJson = storageFile.readAll();
    const nlohmann::json document = nlohmann::json::parse(savedJson.constData(),
                                                          savedJson.constData() + savedJson.size());

    QVERIFY(document.is_object());
    QCOMPARE(document.size(), 2U);
    QVERIFY(document.contains("schemaVersion"));
    QCOMPARE(document.at("schemaVersion").get<int>(), 1);
    QVERIFY(document.contains("songs"));
    QVERIFY(document.at("songs").is_array());
    QCOMPARE(document.at("songs").size(), 1U);

    const nlohmann::json &firstSong = document.at("songs").at(0);
    QVERIFY(firstSong.is_object());
    QCOMPARE(firstSong.size(), 5U);
    QVERIFY(firstSong.contains("filePath"));
    QVERIFY(firstSong.contains("title"));
    QVERIFY(firstSong.contains("artist"));
    QVERIFY(firstSong.contains("album"));
    QVERIFY(firstSong.contains("durationSeconds"));
    QCOMPARE(QString::fromStdString(firstSong.at("filePath").get<std::string>()),
             QStringLiteral("/music/one.mp3"));
    QCOMPARE(QString::fromStdString(firstSong.at("title").get<std::string>()),
             QStringLiteral("One"));
    QCOMPARE(QString::fromStdString(firstSong.at("artist").get<std::string>()),
             QStringLiteral("Artist"));
    QCOMPARE(QString::fromStdString(firstSong.at("album").get<std::string>()),
             QStringLiteral("Album"));
    QCOMPARE(firstSong.at("durationSeconds").get<int>(), 123);
}

void ModelBehaviorTest::jsonSongRepositoryEmptyStoragePathFailsWithError()
{
    JsonSongRepository repository{QString()};
    PlayList playList;
    playList.add(AudioFile(QStringLiteral("/music/song.mp3")));

    QString saveError;
    QVERIFY(!repository.save(playList, &saveError));
    QVERIFY(!saveError.isEmpty());

    QString loadError;
    const std::optional<PlayList> loaded = repository.load(&loadError);
    QVERIFY(!loaded.has_value());
    QVERIFY(!loadError.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositoryCorruptJsonLoadFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::WriteOnly));
    QVERIFY(storageFile.write("{ not valid json") > 0);
    storageFile.close();
    JsonSongRepository repository(storagePath);

    QString errorMessage;
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY(!loaded.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositoryJsonRootNotObjectLoadFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::WriteOnly));
    QVERIFY(storageFile.write("[]") > 0);
    storageFile.close();
    JsonSongRepository repository(storagePath);

    QString errorMessage;
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY(!loaded.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositoryMissingSongsArrayLoadFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::WriteOnly));
    QVERIFY(storageFile.write(R"({"schemaVersion":1})") > 0);
    storageFile.close();
    JsonSongRepository repository(storagePath);

    QString errorMessage;
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY(!loaded.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositorySongEntryMissingFilePathLoadFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::WriteOnly));
    QVERIFY(storageFile.write(R"({"schemaVersion":1,"songs":[{"title":"Missing path"}]})") > 0);
    storageFile.close();
    JsonSongRepository repository(storagePath);

    QString errorMessage;
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY(!loaded.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositorySongEntryEmptyFilePathLoadFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::WriteOnly));
    QVERIFY(storageFile.write(R"({"schemaVersion":1,"songs":[{"filePath":""}]})") > 0);
    storageFile.close();
    JsonSongRepository repository(storagePath);

    QString errorMessage;
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY(!loaded.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositoryNonIntegerDurationLoadFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString storagePath = temporaryDirectory.filePath(QStringLiteral("library.json"));
    QFile storageFile(storagePath);
    QVERIFY(storageFile.open(QIODevice::WriteOnly));
    QVERIFY(storageFile.write(R"({"schemaVersion":1,"songs":[{"filePath":"/music/song.mp3","durationSeconds":1.5}]})") > 0);
    storageFile.close();
    JsonSongRepository repository(storagePath);

    QString errorMessage;
    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY(!loaded.has_value());
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositorySaveInvalidAudioFileFails()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    JsonSongRepository repository(temporaryDirectory.filePath(QStringLiteral("library.json")));
    PlayList playList;
    playList.add(AudioFile());

    QString errorMessage;

    QVERIFY(!repository.save(playList, &errorMessage));
    QVERIFY(!errorMessage.isEmpty());
}

void ModelBehaviorTest::jsonSongRepositoryScanDirectorySaveLoadIntegration()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString scanPath = temporaryDirectory.filePath(QStringLiteral("scan"));
    QVERIFY(QDir().mkpath(scanPath));
    const QString firstPath = QDir(scanPath).filePath(QStringLiteral("First Song.MP3"));
    const QString secondPath = QDir(scanPath).filePath(QStringLiteral("Second Song.flac"));
    for (const QString &path : {firstPath, secondPath}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("fake audio") > 0);
        file.close();
    }

    const ScanResult scanResult = FileScanner::scanDirectory(scanPath);
    QVERIFY2(scanResult.ok(), qPrintable(scanResult.error));
    QCOMPARE(scanResult.playList.size(), 2);
    JsonSongRepository repository(temporaryDirectory.filePath(QStringLiteral("library.json")));
    QString errorMessage;
    QVERIFY2(repository.save(scanResult.playList, &errorMessage), qPrintable(errorMessage));

    const std::optional<PlayList> loaded = repository.load(&errorMessage);

    QVERIFY2(loaded.has_value(), qPrintable(errorMessage));
    QCOMPARE(loaded->size(), scanResult.playList.size());
    for (int index = 0; index < scanResult.playList.size(); ++index) {
        const AudioFile *scannedFile = scanResult.playList.at(index);
        const AudioFile *loadedFile = loaded->at(index);
        QVERIFY(scannedFile != nullptr);
        QVERIFY(loadedFile != nullptr);
        QCOMPARE(loadedFile->filePath(), scannedFile->filePath());
        QCOMPARE(loadedFile->title(), QFileInfo(scannedFile->filePath()).completeBaseName());
        QCOMPARE(loadedFile->artist(), QStringLiteral("Unknown Artist"));
        QCOMPARE(loadedFile->album(), QStringLiteral("Unknown Album"));
        QCOMPARE(loadedFile->durationSeconds(), 0);
    }
}

void ModelBehaviorTest::fileScannerRecognizesSupportedExtensionsCaseInsensitively()
{
    QVERIFY(FileScanner::isSupportedAudioFile(QStringLiteral("/music/track.MP3")));
    QVERIFY(FileScanner::isSupportedAudioFile(QStringLiteral("/music/track.Wav")));
    QVERIFY(FileScanner::isSupportedAudioFile(QStringLiteral("/music/track.flac")));
}

void ModelBehaviorTest::fileScannerRejectsMissingOrUnsupportedFiles()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString textPath = temporaryDirectory.filePath(QStringLiteral("notes.txt"));
    QFile textFile(textPath);
    QVERIFY(textFile.open(QIODevice::WriteOnly));
    QVERIFY(textFile.write("not audio") > 0);
    textFile.close();

    QVERIFY(!FileScanner::scanFile(temporaryDirectory.filePath(QStringLiteral("missing.mp3"))).has_value());
    QVERIFY(!FileScanner::scanFile(textPath).has_value());
}

void ModelBehaviorTest::fileScannerCreatesAudioFileForSupportedFile()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString audioPath = temporaryDirectory.filePath(QStringLiteral("Example Track.MP3"));
    QFile audioFile(audioPath);
    QVERIFY(audioFile.open(QIODevice::WriteOnly));
    QVERIFY(audioFile.write("fake audio bytes") > 0);
    audioFile.close();

    const std::optional<AudioFile> scanned = FileScanner::scanFile(audioPath);

    QVERIFY(scanned.has_value());
    QCOMPARE(scanned->filePath(), QFileInfo(audioPath).absoluteFilePath());
    QCOMPARE(scanned->title(), QStringLiteral("Example Track"));
    QCOMPARE(scanned->artist(), QStringLiteral("Unknown Artist"));
    QCOMPARE(scanned->album(), QStringLiteral("Unknown Album"));
    QCOMPARE(scanned->durationSeconds(), 0);
}

void ModelBehaviorTest::audioFileExtensionLowercasesAndStripsDot()
{
    QCOMPARE(AudioFile(QStringLiteral("/music/Track.MP3")).extension(), QStringLiteral("mp3"));
    QCOMPARE(AudioFile(QStringLiteral("/music/archive.tar.FLAC")).extension(), QStringLiteral("flac"));
    QCOMPARE(AudioFile(QStringLiteral("/music/README")).extension(), QString());
}

void ModelBehaviorTest::fileScannerScanDirectoryReturnsSupportedFilesAndWarnings()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    QFile mp3File(temporaryDirectory.filePath(QStringLiteral("song.mp3")));
    QVERIFY(mp3File.open(QIODevice::WriteOnly));
    QVERIFY(mp3File.write("fake audio") > 0);
    mp3File.close();

    QFile textFile(temporaryDirectory.filePath(QStringLiteral("notes.txt")));
    QVERIFY(textFile.open(QIODevice::WriteOnly));
    QVERIFY(textFile.write("not audio") > 0);
    textFile.close();

    const ScanResult result = FileScanner::scanDirectory(temporaryDirectory.path());

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.playList.size(), 1);
    QVERIFY(result.playList.at(0) != nullptr);
    QCOMPARE(result.playList.at(0)->filePath(), QFileInfo(mp3File.fileName()).absoluteFilePath());
    QCOMPARE(result.playList.at(0)->title(), QStringLiteral("song"));
    QCOMPARE(result.playList.at(0)->artist(), QStringLiteral("Unknown Artist"));
    QCOMPARE(result.playList.at(0)->album(), QStringLiteral("Unknown Album"));
    QCOMPARE(result.playList.at(0)->durationSeconds(), 0);
    QCOMPARE(result.warnings.size(), 1);
    QVERIFY(result.warnings.at(0).contains(QStringLiteral("notes.txt")));
}

void ModelBehaviorTest::fileScannerScanDirectoryRecursiveFlagControlsSubdirectoryScanning()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString rootPath = temporaryDirectory.filePath(QStringLiteral("root.mp3"));
    QFile rootFile(rootPath);
    QVERIFY(rootFile.open(QIODevice::WriteOnly));
    QVERIFY(rootFile.write("fake audio") > 0);
    rootFile.close();

    const QString subDirectoryPath = temporaryDirectory.filePath(QStringLiteral("nested"));
    QVERIFY(QDir().mkpath(subDirectoryPath));
    const QString nestedPath = QDir(subDirectoryPath).filePath(QStringLiteral("nested.flac"));
    QFile nestedFile(nestedPath);
    QVERIFY(nestedFile.open(QIODevice::WriteOnly));
    QVERIFY(nestedFile.write("fake audio") > 0);
    nestedFile.close();

    const ScanResult defaultResult = FileScanner::scanDirectory(temporaryDirectory.path());
    QVERIFY2(defaultResult.ok(), qPrintable(defaultResult.error));
    QCOMPARE(defaultResult.playList.size(), 1);
    QVERIFY(defaultResult.playList.containsPath(QFileInfo(rootPath).absoluteFilePath()));
    QVERIFY(!defaultResult.playList.containsPath(QFileInfo(nestedPath).absoluteFilePath()));

    const ScanResult recursiveResult = FileScanner::scanDirectory(temporaryDirectory.path(), true);
    QVERIFY2(recursiveResult.ok(), qPrintable(recursiveResult.error));
    QCOMPARE(recursiveResult.playList.size(), 2);
    QVERIFY(recursiveResult.playList.containsPath(QFileInfo(rootPath).absoluteFilePath()));
    QVERIFY(recursiveResult.playList.containsPath(QFileInfo(nestedPath).absoluteFilePath()));
}

void ModelBehaviorTest::fileScannerScanDirectoryHandlesExtensionsCaseInsensitively()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString mp3Path = temporaryDirectory.filePath(QStringLiteral("UPPER.MP3"));
    const QString wavPath = temporaryDirectory.filePath(QStringLiteral("Mixed.WaV"));
    QFile mp3File(mp3Path);
    QVERIFY(mp3File.open(QIODevice::WriteOnly));
    QVERIFY(mp3File.write("fake audio") > 0);
    mp3File.close();
    QFile wavFile(wavPath);
    QVERIFY(wavFile.open(QIODevice::WriteOnly));
    QVERIFY(wavFile.write("fake audio") > 0);
    wavFile.close();

    const ScanResult result = FileScanner::scanDirectory(temporaryDirectory.path());

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.playList.size(), 2);
    QVERIFY(result.playList.containsPath(QFileInfo(mp3Path).absoluteFilePath()));
    QVERIFY(result.playList.containsPath(QFileInfo(wavPath).absoluteFilePath()));
}

void ModelBehaviorTest::fileScannerScanDirectoryMissingDirectoryIsFatal()
{
    const ScanResult emptyPathResult = FileScanner::scanDirectory(QString());
    QVERIFY(!emptyPathResult.ok());
    QVERIFY(!emptyPathResult.error.isEmpty());
    QCOMPARE(emptyPathResult.playList.size(), 0);

    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const ScanResult missingPathResult = FileScanner::scanDirectory(temporaryDirectory.filePath(QStringLiteral("missing")));

    QVERIFY(!missingPathResult.ok());
    QVERIFY(!missingPathResult.error.isEmpty());
    QCOMPARE(missingPathResult.playList.size(), 0);
}

void ModelBehaviorTest::fileScannerScanDirectoryFilePathIsFatal()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString filePath = temporaryDirectory.filePath(QStringLiteral("song.mp3"));
    QFile audioFile(filePath);
    QVERIFY(audioFile.open(QIODevice::WriteOnly));
    QVERIFY(audioFile.write("fake audio") > 0);
    audioFile.close();

    const ScanResult result = FileScanner::scanDirectory(filePath);

    QVERIFY(!result.ok());
    QVERIFY(!result.error.isEmpty());
    QCOMPARE(result.playList.size(), 0);
}

void ModelBehaviorTest::fileScannerScanDirectoryEmptyDirectoryIsOk()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const ScanResult result = FileScanner::scanDirectory(temporaryDirectory.path());

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.playList.size(), 0);
    QVERIFY(result.warnings.isEmpty());
}

void ModelBehaviorTest::fileScannerScanDirectoryOrdersByAbsolutePath()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());

    const QString bPath = temporaryDirectory.filePath(QStringLiteral("b.wav"));
    const QString aPath = temporaryDirectory.filePath(QStringLiteral("a.mp3"));
    const QString cPath = temporaryDirectory.filePath(QStringLiteral("c.flac"));
    for (const QString &path : {bPath, cPath, aPath}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QVERIFY(file.write("fake audio") > 0);
        file.close();
    }

    const ScanResult result = FileScanner::scanDirectory(temporaryDirectory.path());

    QVERIFY2(result.ok(), qPrintable(result.error));
    QCOMPARE(result.playList.size(), 3);
    QCOMPARE(result.playList.at(0)->filePath(), QFileInfo(aPath).absoluteFilePath());
    QCOMPARE(result.playList.at(1)->filePath(), QFileInfo(bPath).absoluteFilePath());
    QCOMPARE(result.playList.at(2)->filePath(), QFileInfo(cPath).absoluteFilePath());
}

void ModelBehaviorTest::fileScannerScanDirectoryUnreadableDirectoryIsFatal()
{
    QTemporaryDir temporaryDirectory;
    QVERIFY(temporaryDirectory.isValid());
    const QString unreadablePath = temporaryDirectory.filePath(QStringLiteral("unreadable"));
    QVERIFY(QDir().mkpath(unreadablePath));
    QFileInfo unreadableInfo(unreadablePath);
    const QFileDevice::Permissions originalPermissions = unreadableInfo.permissions();
    QVERIFY(QFile::setPermissions(unreadablePath, QFileDevice::Permissions{}));
    unreadableInfo.refresh();
    if (unreadableInfo.isReadable()) {
        QFile::setPermissions(unreadablePath, originalPermissions);
        QSKIP("Platform still reports the directory as readable after removing read permissions.");
    }

    const ScanResult result = FileScanner::scanDirectory(unreadablePath);

    QFile::setPermissions(unreadablePath, originalPermissions);
    QVERIFY(!result.ok());
    QVERIFY(!result.error.isEmpty());
    QCOMPARE(result.playList.size(), 0);
}

QTEST_MAIN(ModelBehaviorTest)

#include "model_behavior_test.moc"
