#include "Model/Service/ProcessingUseCase.h"

#include <QFileInfo>

namespace AudioPlayer::Model::Service {

namespace {

QString extensionFor(AudioPlayer::Model::ProcessingOutputFormat format)
{
    return AudioPlayer::Model::toString(format);
}

} // namespace

ProcessingUseCase::ProcessingUseCase(ITranscodingBackend *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
    Q_ASSERT(m_backend != nullptr);
    connect(m_backend, &ITranscodingBackend::progressChanged, this, &ProcessingUseCase::handleProgress);
    connect(m_backend, &ITranscodingBackend::transcodeSucceeded, this, &ProcessingUseCase::handleSucceeded);
    connect(m_backend, &ITranscodingBackend::transcodeFailed, this, &ProcessingUseCase::handleFailed);
    connect(m_backend, &ITranscodingBackend::transcodeCanceled, this, &ProcessingUseCase::handleCanceled);
}

const QVector<AudioPlayer::Model::ProcessingTask> &ProcessingUseCase::tasks() const noexcept
{
    return m_tasks;
}

bool ProcessingUseCase::hasPendingOrRunningTasks() const noexcept
{
    for (const auto &task : m_tasks) {
        if (task.canCancel()) {
            return true;
        }
    }
    return false;
}

bool ProcessingUseCase::running() const noexcept
{
    return m_backend != nullptr && m_backend->busy();
}

ProcessingEnqueueResult ProcessingUseCase::enqueueBatch(const ProcessingEnqueueRequest &request)
{
    if (request.inputFilePaths.isEmpty()) {
        return {{}, {}, QStringLiteral("No input files selected.")};
    }
    if (request.outputDirectory.trimmed().isEmpty()) {
        return {{}, {}, QStringLiteral("No output directory selected.")};
    }
    if (!request.timestamp.isValid()) {
        return {{}, {}, QStringLiteral("Invalid task timestamp.")};
    }

    QVector<AudioPlayer::Model::ProcessingTask> created;
    created.reserve(request.inputFilePaths.size());

    for (const QString &inputPath : request.inputFilePaths) {
        if (inputPath.trimmed().isEmpty()) {
            continue;
        }

        const QString outputPath = outputPathFor(inputPath, request.outputDirectory, request.outputFormat);
        AudioPlayer::Model::ProcessingTask task(nextTaskId(),
                                                AudioPlayer::Model::ProcessingTaskType::Transcode,
                                                inputPath,
                                                outputPath,
                                                request.outputFormat,
                                                request.timestamp);
        m_reservedOutputPaths.insert(outputPath);
        m_tasks.push_back(task);
        created.push_back(std::move(task));
    }

    if (created.isEmpty()) {
        return {{}, {}, QStringLiteral("No valid input files selected.")};
    }

    emit tasksChanged();
    startNextPending();
    return {created, QStringLiteral("Transcode tasks queued."), {}};
}

bool ProcessingUseCase::cancelTask(const QString &taskId, const QDateTime &timestamp)
{
    const int index = indexOfTask(taskId);
    if (index < 0 || !timestamp.isValid()) {
        return false;
    }

    auto &task = m_tasks[index];
    if (task.status() == AudioPlayer::Model::ProcessingTaskStatus::Pending) {
        const bool canceled = task.cancel(timestamp);
        if (canceled) {
            markTaskUpdated(index);
        }
        return canceled;
    }

    if (task.status() == AudioPlayer::Model::ProcessingTaskStatus::Running) {
        m_backend->cancelCurrent();
        return true;
    }

    return false;
}

void ProcessingUseCase::cancelAll(const QDateTime &timestamp)
{
    if (!timestamp.isValid()) {
        return;
    }

    bool changed = false;
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].status() == AudioPlayer::Model::ProcessingTaskStatus::Pending) {
            if (m_tasks[i].cancel(timestamp)) {
                emit taskUpdated(m_tasks[i].id());
                changed = true;
            }
        }
    }

    if (m_backend->busy()) {
        m_backend->cancelCurrent();
    }

    if (changed) {
        emit tasksChanged();
    }
}

void ProcessingUseCase::handleProgress(TranscodingProgress progress)
{
    const int index = indexOfTask(progress.taskId);
    if (index < 0) {
        return;
    }

    if (progress.indeterminate) {
        m_tasks[index].setProgressIndeterminate();
    } else {
        m_tasks[index].setProgressPercent(progress.percent);
    }
    markTaskUpdated(index);
}

void ProcessingUseCase::handleSucceeded(TranscodingResult result)
{
    const int index = indexOfTask(result.taskId);
    if (index >= 0 && m_tasks[index].markSucceeded(result.outputFilePath, QDateTime::currentDateTimeUtc())) {
        markTaskUpdated(index);
    }
    startNextPending();
}

void ProcessingUseCase::handleFailed(TranscodingError error)
{
    const int index = indexOfTask(error.taskId);
    if (index >= 0 && m_tasks[index].markFailed(error.message, error.technicalDetails, QDateTime::currentDateTimeUtc())) {
        markTaskUpdated(index);
    }
    startNextPending();
}

void ProcessingUseCase::handleCanceled(const QString &taskId)
{
    const int index = indexOfTask(taskId);
    if (index >= 0 && m_tasks[index].cancel(QDateTime::currentDateTimeUtc())) {
        markTaskUpdated(index);
    }
    startNextPending();
}

QString ProcessingUseCase::nextTaskId() noexcept
{
    return QStringLiteral("processing-task-%1").arg(m_nextTaskNumber++);
}

QString ProcessingUseCase::outputPathFor(const QString &inputFilePath,
                                         const QString &outputDirectory,
                                         AudioPlayer::Model::ProcessingOutputFormat format) const
{
    const QFileInfo inputInfo(inputFilePath);
    const QString baseName = inputInfo.completeBaseName().isEmpty() ? inputInfo.baseName() : inputInfo.completeBaseName();
    const QString safeBaseName = baseName.isEmpty() ? QStringLiteral("output") : baseName;
    const QString candidate = QDir(outputDirectory).filePath(safeBaseName + QStringLiteral(".") + extensionFor(format));
    return uniqueOutputPath(candidate);
}

QString ProcessingUseCase::uniqueOutputPath(const QString &basePath) const
{
    QFileInfo info(basePath);
    const QDir directory = info.dir();
    const QString suffix = info.suffix();
    const QString completeBaseName = info.completeBaseName();

    QString candidate = basePath;
    int counter = 1;
    while (QFileInfo::exists(candidate) || m_reservedOutputPaths.contains(candidate)) {
        candidate = directory.filePath(QStringLiteral("%1 (%2)%3%4")
                                           .arg(completeBaseName)
                                           .arg(counter++)
                                           .arg(suffix.isEmpty() ? QString() : QStringLiteral("."))
                                           .arg(suffix));
    }
    return candidate;
}

int ProcessingUseCase::indexOfTask(const QString &taskId) const noexcept
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].id() == taskId) {
            return i;
        }
    }
    return -1;
}

int ProcessingUseCase::firstPendingTaskIndex() const noexcept
{
    for (int i = 0; i < m_tasks.size(); ++i) {
        if (m_tasks[i].status() == AudioPlayer::Model::ProcessingTaskStatus::Pending) {
            return i;
        }
    }
    return -1;
}

void ProcessingUseCase::startNextPending()
{
    if (m_backend->busy()) {
        return;
    }

    const int index = firstPendingTaskIndex();
    if (index < 0) {
        emit queueDrained();
        return;
    }

    auto &task = m_tasks[index];
    if (!task.markRunning(QDateTime::currentDateTimeUtc())) {
        return;
    }
    markTaskUpdated(index);

    m_backend->startTranscode(TranscodingRequest{task.id(), task.inputFilePath(), task.outputFilePath(), task.outputFormat()});
}

void ProcessingUseCase::markTaskUpdated(int index)
{
    if (index < 0 || index >= m_tasks.size()) {
        return;
    }

    emit taskUpdated(m_tasks[index].id());
    emit tasksChanged();
}

} // namespace AudioPlayer::Model::Service
