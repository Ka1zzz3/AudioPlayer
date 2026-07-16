#include "Model/ProcessingTask.h"
#include "Model/ProcessingTypes.h"

#include <QtTest/QtTest>

using AudioPlayer::Model::ProcessingOutputFormat;
using AudioPlayer::Model::ProcessingTask;
using AudioPlayer::Model::ProcessingTaskStatus;
using AudioPlayer::Model::ProcessingTaskType;

class ProcessingTaskModelTest : public QObject
{
    Q_OBJECT

private slots:
    void transcodeTaskStartsPendingWithValidFields();
    void runningTaskAcceptsDeterminateAndIndeterminateProgress();
    void progressIsClampedToUserFacingRange();
    void successRequiresRunningTaskAndStoresResult();
    void failureStoresUserMessageAndTechnicalDetails();
    void pendingAndRunningTasksCanBeCanceled();
    void terminalTasksRejectFurtherMutation();
    void enumValuesHaveStableDisplayStrings();
};

namespace {

QDateTime utcTime(int seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC);
}

ProcessingTask makeTask()
{
    return ProcessingTask(QStringLiteral("task-1"),
                          ProcessingTaskType::Transcode,
                          QStringLiteral("/music/input.flac"),
                          QStringLiteral("/out/input.mp3"),
                          ProcessingOutputFormat::Mp3,
                          utcTime(1000));
}

} // namespace

void ProcessingTaskModelTest::transcodeTaskStartsPendingWithValidFields()
{
    const ProcessingTask task = makeTask();

    QVERIFY(task.isValid());
    QCOMPARE(task.id(), QStringLiteral("task-1"));
    QVERIFY(task.type() == ProcessingTaskType::Transcode);
    QCOMPARE(task.inputFilePath(), QStringLiteral("/music/input.flac"));
    QCOMPARE(task.outputFilePath(), QStringLiteral("/out/input.mp3"));
    QVERIFY(task.outputFormat() == ProcessingOutputFormat::Mp3);
    QVERIFY(task.status() == ProcessingTaskStatus::Pending);
    QCOMPARE(task.progressPercent(), 0);
    QVERIFY(!task.progressIndeterminate());
    QVERIFY(task.canCancel());
    QVERIFY(!task.isTerminal());
    QCOMPARE(task.createdAt(), utcTime(1000));
    QVERIFY(!task.startedAt().isValid());
    QVERIFY(!task.finishedAt().isValid());
}

void ProcessingTaskModelTest::runningTaskAcceptsDeterminateAndIndeterminateProgress()
{
    ProcessingTask task = makeTask();

    QVERIFY(task.markRunning(utcTime(1001)));
    QVERIFY(task.status() == ProcessingTaskStatus::Running);
    QCOMPARE(task.startedAt(), utcTime(1001));

    QVERIFY(task.setProgressPercent(42));
    QCOMPARE(task.progressPercent(), 42);
    QVERIFY(!task.progressIndeterminate());

    QVERIFY(task.setProgressIndeterminate());
    QCOMPARE(task.progressPercent(), 42);
    QVERIFY(task.progressIndeterminate());
}

void ProcessingTaskModelTest::progressIsClampedToUserFacingRange()
{
    ProcessingTask task = makeTask();

    QVERIFY(task.markRunning(utcTime(1001)));
    QVERIFY(task.setProgressPercent(-10));
    QCOMPARE(task.progressPercent(), 0);
    QVERIFY(task.setProgressPercent(150));
    QCOMPARE(task.progressPercent(), 100);
}

void ProcessingTaskModelTest::successRequiresRunningTaskAndStoresResult()
{
    ProcessingTask task = makeTask();

    QVERIFY(!task.markSucceeded(QStringLiteral("/out/input.mp3"), utcTime(1002)));
    QVERIFY(task.markRunning(utcTime(1001)));
    QVERIFY(task.setProgressIndeterminate());

    QVERIFY(task.markSucceeded(QStringLiteral("/out/input.mp3"), utcTime(1002)));

    QVERIFY(task.status() == ProcessingTaskStatus::Succeeded);
    QCOMPARE(task.progressPercent(), 100);
    QVERIFY(!task.progressIndeterminate());
    QCOMPARE(task.resultFilePath(), QStringLiteral("/out/input.mp3"));
    QCOMPARE(task.finishedAt(), utcTime(1002));
    QVERIFY(task.isTerminal());
    QVERIFY(!task.canCancel());
}

void ProcessingTaskModelTest::failureStoresUserMessageAndTechnicalDetails()
{
    ProcessingTask task = makeTask();

    QVERIFY(task.markRunning(utcTime(1001)));
    QVERIFY(task.markFailed(QStringLiteral("Transcode failed"),
                            QStringLiteral("ffmpeg exited with code 1"),
                            utcTime(1002)));

    QVERIFY(task.status() == ProcessingTaskStatus::Failed);
    QCOMPARE(task.errorMessage(), QStringLiteral("Transcode failed"));
    QCOMPARE(task.technicalDetails(), QStringLiteral("ffmpeg exited with code 1"));
    QVERIFY(task.resultFilePath().isEmpty());
    QCOMPARE(task.finishedAt(), utcTime(1002));
    QVERIFY(task.isTerminal());
}

void ProcessingTaskModelTest::pendingAndRunningTasksCanBeCanceled()
{
    ProcessingTask pending = makeTask();
    QVERIFY(pending.cancel(utcTime(1003)));
    QVERIFY(pending.status() == ProcessingTaskStatus::Canceled);
    QCOMPARE(pending.finishedAt(), utcTime(1003));
    QVERIFY(pending.isTerminal());

    ProcessingTask running = makeTask();
    QVERIFY(running.markRunning(utcTime(1001)));
    QVERIFY(running.cancel(utcTime(1004)));
    QVERIFY(running.status() == ProcessingTaskStatus::Canceled);
    QCOMPARE(running.finishedAt(), utcTime(1004));
}

void ProcessingTaskModelTest::terminalTasksRejectFurtherMutation()
{
    ProcessingTask task = makeTask();

    QVERIFY(task.markRunning(utcTime(1001)));
    QVERIFY(task.markSucceeded(QStringLiteral("/out/input.mp3"), utcTime(1002)));

    QVERIFY(!task.setProgressPercent(10));
    QVERIFY(!task.setProgressIndeterminate());
    QVERIFY(!task.markFailed(QStringLiteral("late failure"), {}, utcTime(1003)));
    QVERIFY(!task.cancel(utcTime(1003)));
    QVERIFY(task.status() == ProcessingTaskStatus::Succeeded);
    QCOMPARE(task.progressPercent(), 100);
}

void ProcessingTaskModelTest::enumValuesHaveStableDisplayStrings()
{
    QCOMPARE(toString(ProcessingTaskType::Transcode), QStringLiteral("transcode"));
    QCOMPARE(toString(ProcessingTaskStatus::Pending), QStringLiteral("pending"));
    QCOMPARE(toString(ProcessingTaskStatus::Running), QStringLiteral("running"));
    QCOMPARE(toString(ProcessingTaskStatus::Succeeded), QStringLiteral("succeeded"));
    QCOMPARE(toString(ProcessingTaskStatus::Failed), QStringLiteral("failed"));
    QCOMPARE(toString(ProcessingTaskStatus::Canceled), QStringLiteral("canceled"));
    QCOMPARE(toString(ProcessingOutputFormat::Mp3), QStringLiteral("mp3"));
    QCOMPARE(toString(ProcessingOutputFormat::Wav), QStringLiteral("wav"));
    QCOMPARE(toString(ProcessingOutputFormat::Flac), QStringLiteral("flac"));
    QVERIFY(AudioPlayer::Model::isTerminal(ProcessingTaskStatus::Succeeded));
    QVERIFY(!AudioPlayer::Model::isTerminal(ProcessingTaskStatus::Running));
}

QTEST_MAIN(ProcessingTaskModelTest)
#include "processing_task_model_test.moc"
