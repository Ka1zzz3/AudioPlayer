#include "Model/Service/ProcessingUseCase.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <utility>

using AudioPlayer::Model::ProcessingOutputFormat;
using AudioPlayer::Model::ProcessingTaskStatus;
using AudioPlayer::Model::Service::ITranscodingBackend;
using AudioPlayer::Model::Service::ProcessingEnqueueRequest;
using AudioPlayer::Model::Service::ProcessingUseCase;
using AudioPlayer::Model::Service::TranscodingError;
using AudioPlayer::Model::Service::TranscodingProgress;
using AudioPlayer::Model::Service::TranscodingRequest;
using AudioPlayer::Model::Service::TranscodingResult;

class UseCaseFakeTranscodingBackend final : public ITranscodingBackend
{
    Q_OBJECT

public:
    explicit UseCaseFakeTranscodingBackend(QObject *parent = nullptr)
        : ITranscodingBackend(parent)
    {
    }

    [[nodiscard]] bool available() const noexcept override { return m_available; }
    [[nodiscard]] QString unavailableReason() const override { return m_unavailableReason; }
    [[nodiscard]] bool busy() const noexcept override { return m_busy; }
    [[nodiscard]] QString currentTaskId() const override { return m_currentRequest.taskId; }
    [[nodiscard]] int startCount() const noexcept { return m_startCount; }
    [[nodiscard]] const QVector<TranscodingRequest> &startedRequests() const noexcept { return m_startedRequests; }

    void setUnavailable(QString reason)
    {
        m_available = false;
        m_unavailableReason = std::move(reason);
    }

public slots:
    void startTranscode(TranscodingRequest request) override
    {
        ++m_startCount;
        m_startedRequests.push_back(request);

        if (!m_available) {
            emit transcodeFailed(TranscodingError{request.taskId, QStringLiteral("Transcoding backend unavailable"), m_unavailableReason});
            return;
        }

        m_busy = true;
        m_currentRequest = std::move(request);
    }

    void cancelCurrent() override
    {
        if (!m_busy) {
            return;
        }

        const QString taskId = m_currentRequest.taskId;
        m_busy = false;
        m_currentRequest = {};
        emit transcodeCanceled(taskId);
    }

    void emitProgress(int percent)
    {
        emit progressChanged(TranscodingProgress{m_currentRequest.taskId, std::clamp(percent, 0, 100), false});
    }

    void emitIndeterminateProgress()
    {
        emit progressChanged(TranscodingProgress{m_currentRequest.taskId, 0, true});
    }

    void succeedCurrent()
    {
        const TranscodingResult result{m_currentRequest.taskId, m_currentRequest.outputFilePath};
        m_busy = false;
        m_currentRequest = {};
        emit transcodeSucceeded(result);
    }

    void failCurrent(QString message = QStringLiteral("Transcode failed"),
                     QString details = QStringLiteral("backend failure"))
    {
        const TranscodingError error{m_currentRequest.taskId, std::move(message), std::move(details)};
        m_busy = false;
        m_currentRequest = {};
        emit transcodeFailed(error);
    }

private:
    bool m_available = true;
    bool m_busy = false;
    QString m_unavailableReason;
    TranscodingRequest m_currentRequest;
    QVector<TranscodingRequest> m_startedRequests;
    int m_startCount = 0;
};

class ProcessingUseCaseTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void enqueueBatchCreatesTasksAndStartsFirstOnly();
    void successfulTaskStartsNextPendingTask();
    void failedTaskContinuesQueue();
    void determinateAndIndeterminateProgressUpdateRunningTask();
    void pendingTaskCanBeCanceledWithoutStartingBackend();
    void runningTaskCancelDelegatesToBackendAndStartsNextTask();
    void cancelAllCancelsPendingAndRunningTasks();
    void outputPathsAreUniqueAndDoNotOverwriteExistingFiles();
    void unavailableBackendMarksTaskFailedAndContinuesQueue();
};

namespace {

QDateTime utcTime(int seconds)
{
    return QDateTime::fromSecsSinceEpoch(seconds, Qt::UTC);
}

ProcessingEnqueueRequest request(QString outputDirectory, QVector<QString> inputs)
{
    return ProcessingEnqueueRequest{std::move(inputs), std::move(outputDirectory), ProcessingOutputFormat::Mp3, utcTime(1000)};
}

} // namespace

void ProcessingUseCaseTest::initTestCase()
{
    qRegisterMetaType<TranscodingRequest>();
    qRegisterMetaType<TranscodingProgress>();
    qRegisterMetaType<TranscodingResult>();
    qRegisterMetaType<TranscodingError>();
}

void ProcessingUseCaseTest::enqueueBatchCreatesTasksAndStartsFirstOnly()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);
    QSignalSpy changedSpy(&useCase, &ProcessingUseCase::tasksChanged);

    const auto result = useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")}));

    QVERIFY(result.ok());
    QCOMPARE(result.tasks.size(), 2);
    QCOMPARE(useCase.tasks().size(), 2);
    QCOMPARE(backend.startCount(), 1);
    QVERIFY(useCase.tasks().at(0).status() == ProcessingTaskStatus::Running);
    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Pending);
    QCOMPARE(backend.currentTaskId(), useCase.tasks().at(0).id());
    QVERIFY(changedSpy.count() >= 2);
}

void ProcessingUseCaseTest::successfulTaskStartsNextPendingTask()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")})).ok());
    const QString firstId = useCase.tasks().at(0).id();
    const QString secondId = useCase.tasks().at(1).id();

    backend.succeedCurrent();

    QVERIFY(useCase.tasks().at(0).status() == ProcessingTaskStatus::Succeeded);
    QCOMPARE(useCase.tasks().at(0).resultFilePath(), QStringLiteral("/out/a.mp3"));
    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Running);
    QCOMPARE(backend.startCount(), 2);
    QCOMPARE(backend.currentTaskId(), secondId);
    QVERIFY(firstId != secondId);
}

void ProcessingUseCaseTest::failedTaskContinuesQueue()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")})).ok());
    backend.failCurrent(QStringLiteral("Unsupported codec"), QStringLiteral("decoder missing"));

    QVERIFY(useCase.tasks().at(0).status() == ProcessingTaskStatus::Failed);
    QCOMPARE(useCase.tasks().at(0).errorMessage(), QStringLiteral("Unsupported codec"));
    QCOMPARE(useCase.tasks().at(0).technicalDetails(), QStringLiteral("decoder missing"));
    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Running);
    QCOMPARE(backend.startCount(), 2);
}

void ProcessingUseCaseTest::determinateAndIndeterminateProgressUpdateRunningTask()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac")})).ok());
    backend.emitProgress(44);

    QCOMPARE(useCase.tasks().first().progressPercent(), 44);
    QVERIFY(!useCase.tasks().first().progressIndeterminate());

    backend.emitIndeterminateProgress();

    QCOMPARE(useCase.tasks().first().progressPercent(), 44);
    QVERIFY(useCase.tasks().first().progressIndeterminate());
}

void ProcessingUseCaseTest::pendingTaskCanBeCanceledWithoutStartingBackend()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")})).ok());
    const QString pendingId = useCase.tasks().at(1).id();

    QVERIFY(useCase.cancelTask(pendingId, utcTime(2000)));

    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Canceled);
    QCOMPARE(backend.startCount(), 1);
}

void ProcessingUseCaseTest::runningTaskCancelDelegatesToBackendAndStartsNextTask()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")})).ok());
    const QString runningId = useCase.tasks().at(0).id();

    QVERIFY(useCase.cancelTask(runningId, utcTime(2000)));

    QVERIFY(useCase.tasks().at(0).status() == ProcessingTaskStatus::Canceled);
    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Running);
    QCOMPARE(backend.startCount(), 2);
}

void ProcessingUseCaseTest::cancelAllCancelsPendingAndRunningTasks()
{
    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")})).ok());
    useCase.cancelAll(utcTime(2000));

    QVERIFY(useCase.tasks().at(0).status() == ProcessingTaskStatus::Canceled);
    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Canceled);
    QVERIFY(!useCase.hasPendingOrRunningTasks());
    QVERIFY(!backend.busy());
}

void ProcessingUseCaseTest::outputPathsAreUniqueAndDoNotOverwriteExistingFiles()
{
    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    QFile existing(QDir(temporaryDir.path()).filePath(QStringLiteral("song.mp3")));
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.close();

    UseCaseFakeTranscodingBackend backend;
    ProcessingUseCase useCase(&backend);

    const auto result = useCase.enqueueBatch(request(temporaryDir.path(), {QStringLiteral("/music/song.flac"), QStringLiteral("/other/song.wav")}));

    QVERIFY(result.ok());
    QCOMPARE(useCase.tasks().at(0).outputFilePath(), QDir(temporaryDir.path()).filePath(QStringLiteral("song (1).mp3")));
    QCOMPARE(useCase.tasks().at(1).outputFilePath(), QDir(temporaryDir.path()).filePath(QStringLiteral("song (2).mp3")));
}

void ProcessingUseCaseTest::unavailableBackendMarksTaskFailedAndContinuesQueue()
{
    UseCaseFakeTranscodingBackend backend;
    backend.setUnavailable(QStringLiteral("ffmpeg not found"));
    ProcessingUseCase useCase(&backend);
    QSignalSpy drainedSpy(&useCase, &ProcessingUseCase::queueDrained);

    QVERIFY(useCase.enqueueBatch(request(QStringLiteral("/out"), {QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")})).ok());

    QCOMPARE(backend.startCount(), 2);
    QVERIFY(useCase.tasks().at(0).status() == ProcessingTaskStatus::Failed);
    QVERIFY(useCase.tasks().at(1).status() == ProcessingTaskStatus::Failed);
    QVERIFY(!useCase.hasPendingOrRunningTasks());
    QVERIFY(drainedSpy.count() >= 1);
}

QTEST_MAIN(ProcessingUseCaseTest)
#include "processing_usecase_test.moc"
