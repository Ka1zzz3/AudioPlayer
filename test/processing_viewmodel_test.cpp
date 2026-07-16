#include "ViewModel/ProcessingViewModel.h"
#include "ViewModel/ProcessingTaskListModel.h"

#include "Model/Service/ITranscodingBackend.h"

#include <QtTest/QtTest>

using AudioPlayer::Model::ProcessingOutputFormat;
using AudioPlayer::Model::Service::ITranscodingBackend;
using AudioPlayer::Model::Service::ProcessingUseCase;
using AudioPlayer::Model::Service::TranscodingError;
using AudioPlayer::Model::Service::TranscodingProgress;
using AudioPlayer::Model::Service::TranscodingRequest;
using AudioPlayer::Model::Service::TranscodingResult;
using AudioPlayer::ViewModel::ProcessingTaskListModel;
using AudioPlayer::ViewModel::ProcessingViewModel;

class ViewModelFakeTranscodingBackend final : public ITranscodingBackend
{
    Q_OBJECT

public:
    [[nodiscard]] bool available() const noexcept override { return true; }
    [[nodiscard]] QString unavailableReason() const override { return {}; }
    [[nodiscard]] bool busy() const noexcept override { return m_busy; }
    [[nodiscard]] QString currentTaskId() const override { return m_request.taskId; }

public slots:
    void startTranscode(TranscodingRequest request) override
    {
        ++startCount;
        m_busy = true;
        m_request = std::move(request);
    }

    void cancelCurrent() override
    {
        const QString taskId = m_request.taskId;
        m_busy = false;
        m_request = {};
        emit transcodeCanceled(taskId);
    }

    void emitProgress(int percent) { emit progressChanged(TranscodingProgress{m_request.taskId, percent, false}); }
    void succeed()
    {
        const TranscodingResult result{m_request.taskId, m_request.outputFilePath};
        m_busy = false;
        m_request = {};
        emit transcodeSucceeded(result);
    }

public:
    int startCount = 0;

private:
    bool m_busy = false;
    TranscodingRequest m_request;
};

class ProcessingViewModelTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void enqueueCommandCreatesTasksAndUpdatesStatus();
    void taskListModelExposesTaskRoles();
    void cancelSelectedCommandCancelsSelectedTask();
    void cancelAllCommandClearsActiveFlag();
    void invalidFormatIsRejectedWithVisibleError();
};

void ProcessingViewModelTest::initTestCase()
{
    qRegisterMetaType<TranscodingRequest>();
    qRegisterMetaType<TranscodingProgress>();
    qRegisterMetaType<TranscodingResult>();
    qRegisterMetaType<TranscodingError>();
}

void ProcessingViewModelTest::enqueueCommandCreatesTasksAndUpdatesStatus()
{
    auto backend = std::make_unique<ViewModelFakeTranscodingBackend>();
    auto *backendRaw = backend.get();
    auto useCase = std::make_shared<ProcessingUseCase>(backend.release());
    ProcessingViewModel viewModel(useCase);
    QSignalSpy statusSpy(&viewModel, &ProcessingViewModel::statusMessageChanged);
    QSignalSpy activeSpy(&viewModel, &ProcessingViewModel::hasPendingOrRunningTasksChanged);

    viewModel.setInputFilePaths({QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")});
    viewModel.setOutputDirectory(QStringLiteral("/out"));
    viewModel.setOutputFormat(QStringLiteral("flac"));

    QVERIFY(viewModel.enqueueCommand()->execute());

    QCOMPARE(viewModel.tasks()->rowCount(), 2);
    QVERIFY(viewModel.hasPendingOrRunningTasks());
    QCOMPARE(viewModel.statusMessage(), QStringLiteral("Transcode tasks queued."));
    QCOMPARE(backendRaw->startCount, 1);
    QVERIFY(statusSpy.count() >= 1);
    QVERIFY(activeSpy.count() >= 1);
}

void ProcessingViewModelTest::taskListModelExposesTaskRoles()
{
    auto backend = std::make_unique<ViewModelFakeTranscodingBackend>();
    auto useCase = std::make_shared<ProcessingUseCase>(backend.release());
    ProcessingViewModel viewModel(useCase);

    viewModel.setInputFilePaths({QStringLiteral("/music/a.flac")});
    viewModel.setOutputDirectory(QStringLiteral("/out"));
    QVERIFY(viewModel.enqueueCommand()->execute());

    const QModelIndex index = viewModel.tasks()->index(0, 0);
    QCOMPARE(viewModel.tasks()->data(index, ProcessingTaskListModel::InputPathRole).toString(), QStringLiteral("/music/a.flac"));
    QCOMPARE(viewModel.tasks()->data(index, ProcessingTaskListModel::OutputPathRole).toString(), QStringLiteral("/out/a.mp3"));
    QCOMPARE(viewModel.tasks()->data(index, ProcessingTaskListModel::StatusRole).toString(), QStringLiteral("running"));
    QCOMPARE(viewModel.tasks()->data(index, ProcessingTaskListModel::ProgressPercentRole).toInt(), 0);
}

void ProcessingViewModelTest::cancelSelectedCommandCancelsSelectedTask()
{
    auto backend = std::make_unique<ViewModelFakeTranscodingBackend>();
    auto useCase = std::make_shared<ProcessingUseCase>(backend.release());
    ProcessingViewModel viewModel(useCase);

    viewModel.setInputFilePaths({QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")});
    viewModel.setOutputDirectory(QStringLiteral("/out"));
    QVERIFY(viewModel.enqueueCommand()->execute());
    viewModel.setSelectedTaskRow(1);

    QVERIFY(viewModel.cancelSelectedCommand()->execute());

    const QModelIndex index = viewModel.tasks()->index(1, 0);
    QCOMPARE(viewModel.tasks()->data(index, ProcessingTaskListModel::StatusRole).toString(), QStringLiteral("canceled"));
    QVERIFY(viewModel.hasPendingOrRunningTasks());
}

void ProcessingViewModelTest::cancelAllCommandClearsActiveFlag()
{
    auto backend = std::make_unique<ViewModelFakeTranscodingBackend>();
    auto useCase = std::make_shared<ProcessingUseCase>(backend.release());
    ProcessingViewModel viewModel(useCase);

    viewModel.setInputFilePaths({QStringLiteral("/music/a.flac"), QStringLiteral("/music/b.wav")});
    viewModel.setOutputDirectory(QStringLiteral("/out"));
    QVERIFY(viewModel.enqueueCommand()->execute());

    QVERIFY(viewModel.cancelAllCommand()->execute());

    QVERIFY(!viewModel.hasPendingOrRunningTasks());
    QCOMPARE(viewModel.statusMessage(), QStringLiteral("Pending and running processing tasks canceled."));
}

void ProcessingViewModelTest::invalidFormatIsRejectedWithVisibleError()
{
    auto backend = std::make_unique<ViewModelFakeTranscodingBackend>();
    auto useCase = std::make_shared<ProcessingUseCase>(backend.release());
    ProcessingViewModel viewModel(useCase);
    QSignalSpy errorSpy(&viewModel, &ProcessingViewModel::lastErrorChanged);

    viewModel.setOutputFormat(QStringLiteral("aac"));

    QCOMPARE(viewModel.outputFormat(), QStringLiteral("mp3"));
    QCOMPARE(viewModel.lastError(), QStringLiteral("Unsupported output format."));
    QCOMPARE(errorSpy.count(), 1);
}

QTEST_MAIN(ProcessingViewModelTest)
#include "processing_viewmodel_test.moc"
