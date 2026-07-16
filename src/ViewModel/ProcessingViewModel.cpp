#include "ViewModel/ProcessingViewModel.h"

namespace AudioPlayer::ViewModel {

namespace {

QDateTime nowUtc()
{
    return QDateTime::currentDateTimeUtc();
}

} // namespace

ProcessingViewModel::ProcessingViewModel(std::shared_ptr<Model::Service::ProcessingUseCase> useCase, QObject *parent)
    : ProcessingViewModelProtocol(parent)
    , m_useCase(std::move(useCase))
    , m_enqueueCommand(QStringLiteral("enqueueProcessing"), [this] { return executeEnqueue(); }, this)
    , m_cancelSelectedCommand(QStringLiteral("cancelSelectedProcessing"), [this] { return executeCancelSelected(); }, this)
    , m_cancelAllCommand(QStringLiteral("cancelAllProcessing"), [this] { return executeCancelAll(); }, this)
{
    Q_ASSERT(m_useCase != nullptr);
    connect(m_useCase.get(), &Model::Service::ProcessingUseCase::tasksChanged, this, &ProcessingViewModel::refreshTasks);
    refreshTasks();
    refreshActiveFlag();
}

QAbstractItemModel *ProcessingViewModel::tasks() noexcept
{
    return &m_tasks;
}

const QStringList &ProcessingViewModel::inputFilePaths() const noexcept
{
    return m_inputFilePaths;
}

void ProcessingViewModel::setInputFilePaths(QStringList paths)
{
    if (m_inputFilePaths == paths) {
        return;
    }
    m_inputFilePaths = std::move(paths);
    emit inputFilePathsChanged();
}

const QString &ProcessingViewModel::outputDirectory() const noexcept
{
    return m_outputDirectory;
}

void ProcessingViewModel::setOutputDirectory(QString directory)
{
    if (m_outputDirectory == directory) {
        return;
    }
    m_outputDirectory = std::move(directory);
    emit outputDirectoryChanged();
}

const QString &ProcessingViewModel::outputFormat() const noexcept
{
    return m_outputFormat;
}

void ProcessingViewModel::setOutputFormat(QString format)
{
    const QString normalized = format.trimmed().toLower();
    if (normalized != QStringLiteral("mp3") && normalized != QStringLiteral("wav") && normalized != QStringLiteral("flac")) {
        setLastError(QStringLiteral("Unsupported output format."));
        return;
    }
    if (m_outputFormat == normalized) {
        return;
    }
    m_outputFormat = normalized;
    emit outputFormatChanged();
}

int ProcessingViewModel::selectedTaskRow() const noexcept
{
    return m_selectedTaskRow;
}

void ProcessingViewModel::setSelectedTaskRow(int row)
{
    if (m_selectedTaskRow == row) {
        return;
    }
    m_selectedTaskRow = row;
    emit selectedTaskRowChanged();
}

bool ProcessingViewModel::hasPendingOrRunningTasks() const noexcept
{
    return m_hasPendingOrRunningTasks;
}

const QString &ProcessingViewModel::lastError() const noexcept
{
    return m_lastError;
}

const QString &ProcessingViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
}

Common::ViewCommand *ProcessingViewModel::enqueueCommand() noexcept
{
    return &m_enqueueCommand;
}

Common::ViewCommand *ProcessingViewModel::cancelSelectedCommand() noexcept
{
    return &m_cancelSelectedCommand;
}

Common::ViewCommand *ProcessingViewModel::cancelAllCommand() noexcept
{
    return &m_cancelAllCommand;
}

void ProcessingViewModel::reportIntegrationWarning(QString warning)
{
    setLastError(std::move(warning));
}

bool ProcessingViewModel::executeEnqueue()
{
    const auto result = m_useCase->enqueueBatch(Model::Service::ProcessingEnqueueRequest{m_inputFilePaths,
                                                                                          m_outputDirectory,
                                                                                          parsedOutputFormat(),
                                                                                          nowUtc()});
    refreshTasks();
    refreshActiveFlag();
    if (!result.ok()) {
        setLastError(result.error);
        return false;
    }
    setLastError({});
    setStatusMessage(result.status);
    return true;
}

bool ProcessingViewModel::executeCancelSelected()
{
    const auto currentTasks = m_useCase->tasks();
    if (m_selectedTaskRow < 0 || m_selectedTaskRow >= currentTasks.size()) {
        setLastError(QStringLiteral("No processing task selected."));
        return false;
    }

    const bool canceled = m_useCase->cancelTask(currentTasks.at(m_selectedTaskRow).id(), nowUtc());
    refreshTasks();
    refreshActiveFlag();
    if (!canceled) {
        setLastError(QStringLiteral("Selected processing task cannot be canceled."));
        return false;
    }
    setLastError({});
    setStatusMessage(QStringLiteral("Processing task canceled."));
    return true;
}

bool ProcessingViewModel::executeCancelAll()
{
    m_useCase->cancelAll(nowUtc());
    refreshTasks();
    refreshActiveFlag();
    setLastError({});
    setStatusMessage(QStringLiteral("Pending and running processing tasks canceled."));
    return true;
}

void ProcessingViewModel::refreshTasks()
{
    m_tasks.setTasks(m_useCase->tasks());
    refreshActiveFlag();
}

void ProcessingViewModel::refreshActiveFlag()
{
    const bool active = m_useCase->hasPendingOrRunningTasks();
    if (m_hasPendingOrRunningTasks == active) {
        return;
    }
    m_hasPendingOrRunningTasks = active;
    emit hasPendingOrRunningTasksChanged();
}

void ProcessingViewModel::setLastError(QString error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = std::move(error);
    emit lastErrorChanged();
}

void ProcessingViewModel::setStatusMessage(QString status)
{
    if (m_statusMessage == status) {
        return;
    }
    m_statusMessage = std::move(status);
    emit statusMessageChanged();
}

Model::ProcessingOutputFormat ProcessingViewModel::parsedOutputFormat() const noexcept
{
    if (m_outputFormat == QStringLiteral("wav")) {
        return Model::ProcessingOutputFormat::Wav;
    }
    if (m_outputFormat == QStringLiteral("flac")) {
        return Model::ProcessingOutputFormat::Flac;
    }
    return Model::ProcessingOutputFormat::Mp3;
}

} // namespace AudioPlayer::ViewModel
