#include "Model/Service/FfmpegTranscodingBackend.h"

#include <QtTest/QtTest>

using AudioPlayer::Model::ProcessingOutputFormat;
using AudioPlayer::Model::Service::FfmpegTranscodingBackend;
using AudioPlayer::Model::Service::TranscodingError;
using AudioPlayer::Model::Service::TranscodingRequest;

class FfmpegTranscodingBackendTest : public QObject
{
    Q_OBJECT

private slots:
    void commandBuilderUsesArgumentListAndNoOverwriteFlag();
    void ffprobeCommandUsesJsonOutputAndKeepsPathAsSingleArgument();
    void durationParserReadsFormatDuration();
    void progressParserMapsOutTimeToPercentAndEndToComplete();
    void processErrorSummaryIsUserReadable();
    void unavailableBackendFailsWithoutStartingProcess();
    void missingInputFailsWithoutStartingProcess();
};

void FfmpegTranscodingBackendTest::commandBuilderUsesArgumentListAndNoOverwriteFlag()
{
    const auto command = FfmpegTranscodingBackend::buildFfmpegCommand(QStringLiteral("ffmpeg"),
                                                                      QStringLiteral("/music/a file;rm.flac"),
                                                                      QStringLiteral("/out/a file.mp3"),
                                                                      ProcessingOutputFormat::Mp3);

    QCOMPARE(command.program, QStringLiteral("ffmpeg"));
    QVERIFY(command.arguments.contains(QStringLiteral("-nostdin")));
    QVERIFY(command.arguments.contains(QStringLiteral("-n")));
    QVERIFY(!command.arguments.contains(QStringLiteral("-y")));
    QVERIFY(command.arguments.contains(QStringLiteral("-progress")));
    QVERIFY(command.arguments.contains(QStringLiteral("pipe:1")));
    QVERIFY(command.arguments.contains(QStringLiteral("libmp3lame")));
    QCOMPARE(command.arguments.at(command.arguments.indexOf(QStringLiteral("-i")) + 1), QStringLiteral("/music/a file;rm.flac"));
    QCOMPARE(command.arguments.last(), QStringLiteral("/out/a file.mp3"));
}

void FfmpegTranscodingBackendTest::ffprobeCommandUsesJsonOutputAndKeepsPathAsSingleArgument()
{
    const auto command = FfmpegTranscodingBackend::buildFfprobeCommand(QStringLiteral("ffprobe"),
                                                                       QStringLiteral("/music/odd name$(x).wav"));

    QCOMPARE(command.program, QStringLiteral("ffprobe"));
    QVERIFY(command.arguments.contains(QStringLiteral("-show_format")));
    QVERIFY(command.arguments.contains(QStringLiteral("-show_streams")));
    QVERIFY(command.arguments.contains(QStringLiteral("json")));
    QCOMPARE(command.arguments.last(), QStringLiteral("/music/odd name$(x).wav"));
}

void FfmpegTranscodingBackendTest::durationParserReadsFormatDuration()
{
    qint64 durationMs = 0;

    QVERIFY(FfmpegTranscodingBackend::parseDurationMs(R"({"format":{"duration":"12.345"}})", &durationMs));

    QCOMPARE(durationMs, 12345);
    QVERIFY(!FfmpegTranscodingBackend::parseDurationMs(R"({"format":{"duration":"0"}})", &durationMs));
}

void FfmpegTranscodingBackendTest::progressParserMapsOutTimeToPercentAndEndToComplete()
{
    int percent = 0;
    bool finished = false;

    QVERIFY(FfmpegTranscodingBackend::parseProgressPercent("out_time_ms=5000000\nprogress=continue\n", 10'000, &percent, &finished));
    QCOMPARE(percent, 50);
    QVERIFY(!finished);

    QVERIFY(FfmpegTranscodingBackend::parseProgressPercent("progress=end\n", 10'000, &percent, &finished));
    QCOMPARE(percent, 100);
    QVERIFY(finished);

    QVERIFY(!FfmpegTranscodingBackend::parseProgressPercent("progress=continue\n", 0, &percent, &finished));
}

void FfmpegTranscodingBackendTest::processErrorSummaryIsUserReadable()
{
    QCOMPARE(FfmpegTranscodingBackend::summarizeProcessError(QProcess::FailedToStart),
             QStringLiteral("Failed to start ffmpeg process."));
    QCOMPARE(FfmpegTranscodingBackend::summarizeProcessError(QProcess::Crashed),
             QStringLiteral("ffmpeg process crashed."));
}

void FfmpegTranscodingBackendTest::unavailableBackendFailsWithoutStartingProcess()
{
    FfmpegTranscodingBackend backend({}, QStringLiteral("ffprobe"));
    QSignalSpy failureSpy(&backend, &FfmpegTranscodingBackend::transcodeFailed);

    backend.startTranscode(TranscodingRequest{QStringLiteral("task-1"), QStringLiteral("/missing/in.flac"), QStringLiteral("/out/in.mp3"), ProcessingOutputFormat::Mp3});

    QVERIFY(!backend.available());
    QVERIFY(!backend.busy());
    QCOMPARE(failureSpy.count(), 1);
    const auto error = failureSpy.takeFirst().at(0).value<TranscodingError>();
    QCOMPARE(error.taskId, QStringLiteral("task-1"));
    QCOMPARE(error.message, QStringLiteral("Transcoding backend unavailable"));
}

void FfmpegTranscodingBackendTest::missingInputFailsWithoutStartingProcess()
{
    FfmpegTranscodingBackend backend(QStringLiteral("ffmpeg"), QStringLiteral("ffprobe"));
    QSignalSpy failureSpy(&backend, &FfmpegTranscodingBackend::transcodeFailed);

    backend.startTranscode(TranscodingRequest{QStringLiteral("task-1"), QStringLiteral("/definitely/missing.flac"), QStringLiteral("/out/missing.mp3"), ProcessingOutputFormat::Mp3});

    QVERIFY(!backend.busy());
    QCOMPARE(failureSpy.count(), 1);
    const auto error = failureSpy.takeFirst().at(0).value<TranscodingError>();
    QCOMPARE(error.message, QStringLiteral("Input file does not exist"));
    QCOMPARE(error.technicalDetails, QStringLiteral("/definitely/missing.flac"));
}

QTEST_MAIN(FfmpegTranscodingBackendTest)
#include "ffmpeg_transcoding_backend_test.moc"
