#include "Model/Service/ITranscodingBackend.h"

#include <QtTest/QtTest>

#include <algorithm>
#include <utility>

using AudioPlayer::Model::ProcessingOutputFormat;
using AudioPlayer::Model::Service::ITranscodingBackend;
using AudioPlayer::Model::Service::TranscodingError;
using AudioPlayer::Model::Service::TranscodingProgress;
using AudioPlayer::Model::Service::TranscodingRequest;
using AudioPlayer::Model::Service::TranscodingResult;

class FakeTranscodingBackend final : public ITranscodingBackend
{
    Q_OBJECT

public:
    explicit FakeTranscodingBackend(QObject *parent = nullptr)
        : ITranscodingBackend(parent)
    {
    }

    [[nodiscard]] bool available() const noexcept override { return m_available; }
    [[nodiscard]] QString unavailableReason() const override { return m_unavailableReason; }
    [[nodiscard]] bool busy() const noexcept override { return m_busy; }
    [[nodiscard]] QString currentTaskId() const override { return m_currentRequest.taskId; }

    void setUnavailable(QString reason)
    {
        m_available = false;
        m_unavailableReason = std::move(reason);
    }

public slots:
    void startTranscode(TranscodingRequest request) override
    {
        if (!m_available) {
            emit transcodeFailed(TranscodingError{request.taskId, QStringLiteral("Transcoding backend unavailable"), m_unavailableReason});
            return;
        }

        if (m_busy) {
            emit transcodeFailed(TranscodingError{request.taskId, QStringLiteral("Transcoding backend is busy"), {}});
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
        clearCurrent();
        emit transcodeCanceled(taskId);
    }

    void emitProgress(int percent)
    {
        if (!m_busy) {
            return;
        }

        emit progressChanged(TranscodingProgress{m_currentRequest.taskId, std::clamp(percent, 0, 100), false});
    }

    void emitIndeterminateProgress()
    {
        if (!m_busy) {
            return;
        }

        emit progressChanged(TranscodingProgress{m_currentRequest.taskId, 0, true});
    }

    void succeed()
    {
        if (!m_busy) {
            return;
        }

        const TranscodingResult result{m_currentRequest.taskId, m_currentRequest.outputFilePath};
        clearCurrent();
        emit transcodeSucceeded(result);
    }

    void fail(QString message, QString technicalDetails)
    {
        if (!m_busy) {
            return;
        }

        const TranscodingError error{m_currentRequest.taskId, std::move(message), std::move(technicalDetails)};
        clearCurrent();
        emit transcodeFailed(error);
    }

private:
    void clearCurrent()
    {
        m_busy = false;
        m_currentRequest = {};
    }

    bool m_available = true;
    bool m_busy = false;
    QString m_unavailableReason;
    TranscodingRequest m_currentRequest;
};

class TranscodingBackendContractTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void fakeBackendStartsSingleRequestAndEmitsDeterminateProgressAndSuccess();
    void fakeBackendEmitsIndeterminateProgress();
    void fakeBackendEmitsFailureWithTechnicalDetails();
    void fakeBackendCancelsRunningRequest();
    void unavailableBackendFailsRequestWithoutBecomingBusy();
    void busyBackendRejectsSecondRequest();
};

namespace {

TranscodingRequest makeRequest(QString taskId = QStringLiteral("task-1"))
{
    return TranscodingRequest{std::move(taskId),
                              QStringLiteral("/music/input.flac"),
                              QStringLiteral("/out/input.mp3"),
                              ProcessingOutputFormat::Mp3};
}

} // namespace

void TranscodingBackendContractTest::initTestCase()
{
    qRegisterMetaType<TranscodingRequest>();
    qRegisterMetaType<TranscodingProgress>();
    qRegisterMetaType<TranscodingResult>();
    qRegisterMetaType<TranscodingError>();
}

void TranscodingBackendContractTest::fakeBackendStartsSingleRequestAndEmitsDeterminateProgressAndSuccess()
{
    FakeTranscodingBackend backend;
    QSignalSpy progressSpy(&backend, &FakeTranscodingBackend::progressChanged);
    QSignalSpy successSpy(&backend, &FakeTranscodingBackend::transcodeSucceeded);

    backend.startTranscode(makeRequest());
    QVERIFY(backend.busy());
    QCOMPARE(backend.currentTaskId(), QStringLiteral("task-1"));

    backend.emitProgress(35);
    backend.succeed();

    QCOMPARE(progressSpy.count(), 1);
    const auto progress = progressSpy.takeFirst().at(0).value<TranscodingProgress>();
    QCOMPARE(progress.taskId, QStringLiteral("task-1"));
    QCOMPARE(progress.percent, 35);
    QVERIFY(!progress.indeterminate);

    QCOMPARE(successSpy.count(), 1);
    const auto result = successSpy.takeFirst().at(0).value<TranscodingResult>();
    QCOMPARE(result.taskId, QStringLiteral("task-1"));
    QCOMPARE(result.outputFilePath, QStringLiteral("/out/input.mp3"));
    QVERIFY(!backend.busy());
}

void TranscodingBackendContractTest::fakeBackendEmitsIndeterminateProgress()
{
    FakeTranscodingBackend backend;
    QSignalSpy progressSpy(&backend, &FakeTranscodingBackend::progressChanged);

    backend.startTranscode(makeRequest());
    backend.emitIndeterminateProgress();

    QCOMPARE(progressSpy.count(), 1);
    const auto progress = progressSpy.takeFirst().at(0).value<TranscodingProgress>();
    QCOMPARE(progress.taskId, QStringLiteral("task-1"));
    QVERIFY(progress.indeterminate);
}

void TranscodingBackendContractTest::fakeBackendEmitsFailureWithTechnicalDetails()
{
    FakeTranscodingBackend backend;
    QSignalSpy failureSpy(&backend, &FakeTranscodingBackend::transcodeFailed);

    backend.startTranscode(makeRequest());
    backend.fail(QStringLiteral("Transcode failed"), QStringLiteral("ffmpeg exited with code 1"));

    QCOMPARE(failureSpy.count(), 1);
    const auto error = failureSpy.takeFirst().at(0).value<TranscodingError>();
    QCOMPARE(error.taskId, QStringLiteral("task-1"));
    QCOMPARE(error.message, QStringLiteral("Transcode failed"));
    QCOMPARE(error.technicalDetails, QStringLiteral("ffmpeg exited with code 1"));
    QVERIFY(!backend.busy());
}

void TranscodingBackendContractTest::fakeBackendCancelsRunningRequest()
{
    FakeTranscodingBackend backend;
    QSignalSpy cancelSpy(&backend, &FakeTranscodingBackend::transcodeCanceled);

    backend.startTranscode(makeRequest());
    backend.cancelCurrent();

    QCOMPARE(cancelSpy.count(), 1);
    QCOMPARE(cancelSpy.takeFirst().at(0).toString(), QStringLiteral("task-1"));
    QVERIFY(!backend.busy());
    QVERIFY(backend.currentTaskId().isEmpty());
}

void TranscodingBackendContractTest::unavailableBackendFailsRequestWithoutBecomingBusy()
{
    FakeTranscodingBackend backend;
    backend.setUnavailable(QStringLiteral("ffmpeg was not found"));
    QSignalSpy failureSpy(&backend, &FakeTranscodingBackend::transcodeFailed);

    backend.startTranscode(makeRequest());

    QVERIFY(!backend.available());
    QVERIFY(!backend.busy());
    QCOMPARE(failureSpy.count(), 1);
    const auto error = failureSpy.takeFirst().at(0).value<TranscodingError>();
    QCOMPARE(error.taskId, QStringLiteral("task-1"));
    QCOMPARE(error.message, QStringLiteral("Transcoding backend unavailable"));
    QCOMPARE(error.technicalDetails, QStringLiteral("ffmpeg was not found"));
}

void TranscodingBackendContractTest::busyBackendRejectsSecondRequest()
{
    FakeTranscodingBackend backend;
    QSignalSpy failureSpy(&backend, &FakeTranscodingBackend::transcodeFailed);

    backend.startTranscode(makeRequest(QStringLiteral("task-1")));
    backend.startTranscode(makeRequest(QStringLiteral("task-2")));

    QVERIFY(backend.busy());
    QCOMPARE(backend.currentTaskId(), QStringLiteral("task-1"));
    QCOMPARE(failureSpy.count(), 1);
    const auto error = failureSpy.takeFirst().at(0).value<TranscodingError>();
    QCOMPARE(error.taskId, QStringLiteral("task-2"));
    QCOMPARE(error.message, QStringLiteral("Transcoding backend is busy"));
}

QTEST_MAIN(TranscodingBackendContractTest)
#include "transcoding_backend_contract_test.moc"
