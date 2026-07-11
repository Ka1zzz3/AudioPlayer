#include "Model/AudioFile.h"
#include "Model/FileScanner.h"
#include "Model/JsonSongRepository.h"
#include "Model/PlayList.h"

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

class ModelBehaviorTest : public QObject
{
    Q_OBJECT

private slots:
    void audioFileClampsNegativeDurationToZero();
    void audioFileUsesTrimmedTitleBeforePathForDisplayTitle();
    void playListTracksCurrentFileAndTotalDuration();
    void jsonSongRepositorySavesAndLoadsPlaylist();
    void fileScannerRecognizesSupportedExtensionsCaseInsensitively();
    void fileScannerRejectsMissingOrUnsupportedFiles();
    void fileScannerCreatesAudioFileForSupportedFile();
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

QTEST_MAIN(ModelBehaviorTest)

#include "model_behavior_test.moc"
