#include "Model/ProcessingTask.h"

#include <algorithm>

namespace AudioPlayer::Model {

ProcessingTask::ProcessingTask(QString id,
                               ProcessingTaskType type,
                               QString inputFilePath,
                               QString outputFilePath,
                               ProcessingOutputFormat outputFormat,
                               QDateTime createdAt)
    : m_id(std::move(id))
    , m_type(type)
    , m_inputFilePath(std::move(inputFilePath))
    , m_outputFilePath(std::move(outputFilePath))
    , m_outputFormat(outputFormat)
    , m_createdAt(std::move(createdAt))
{
}

const QString &ProcessingTask::id() const noexcept
{
    return m_id;
}

ProcessingTaskType ProcessingTask::type() const noexcept
{
    return m_type;
}

const QString &ProcessingTask::inputFilePath() const noexcept
{
    return m_inputFilePath;
}

const QString &ProcessingTask::outputFilePath() const noexcept
{
    return m_outputFilePath;
}

ProcessingOutputFormat ProcessingTask::outputFormat() const noexcept
{
    return m_outputFormat;
}

ProcessingTaskStatus ProcessingTask::status() const noexcept
{
    return m_status;
}

int ProcessingTask::progressPercent() const noexcept
{
    return m_progressPercent;
}

bool ProcessingTask::progressIndeterminate() const noexcept
{
    return m_progressIndeterminate;
}

const QString &ProcessingTask::errorMessage() const noexcept
{
    return m_errorMessage;
}

const QString &ProcessingTask::technicalDetails() const noexcept
{
    return m_technicalDetails;
}

const QString &ProcessingTask::resultFilePath() const noexcept
{
    return m_resultFilePath;
}

const QDateTime &ProcessingTask::createdAt() const noexcept
{
    return m_createdAt;
}

const QDateTime &ProcessingTask::startedAt() const noexcept
{
    return m_startedAt;
}

const QDateTime &ProcessingTask::finishedAt() const noexcept
{
    return m_finishedAt;
}

bool ProcessingTask::isValid() const noexcept
{
    return !m_id.trimmed().isEmpty()
        && !m_inputFilePath.trimmed().isEmpty()
        && !m_outputFilePath.trimmed().isEmpty()
        && m_createdAt.isValid()
        && m_progressPercent >= 0
        && m_progressPercent <= 100;
}

bool ProcessingTask::canCancel() const noexcept
{
    return m_status == ProcessingTaskStatus::Pending || m_status == ProcessingTaskStatus::Running;
}

bool ProcessingTask::isTerminal() const noexcept
{
    return AudioPlayer::Model::isTerminal(m_status);
}

bool ProcessingTask::markRunning(const QDateTime &startedAt)
{
    if (m_status != ProcessingTaskStatus::Pending || !startedAt.isValid()) {
        return false;
    }

    m_status = ProcessingTaskStatus::Running;
    m_startedAt = startedAt;
    m_finishedAt = {};
    m_errorMessage.clear();
    m_technicalDetails.clear();
    m_resultFilePath.clear();
    return true;
}

bool ProcessingTask::setProgressPercent(int percent) noexcept
{
    if (isTerminal()) {
        return false;
    }

    m_progressPercent = std::clamp(percent, 0, 100);
    m_progressIndeterminate = false;
    return true;
}

bool ProcessingTask::setProgressIndeterminate() noexcept
{
    if (isTerminal()) {
        return false;
    }

    m_progressIndeterminate = true;
    return true;
}

bool ProcessingTask::markSucceeded(QString resultFilePath, const QDateTime &finishedAt)
{
    if (m_status != ProcessingTaskStatus::Running || resultFilePath.trimmed().isEmpty() || !finishedAt.isValid()) {
        return false;
    }

    m_status = ProcessingTaskStatus::Succeeded;
    m_progressPercent = 100;
    m_progressIndeterminate = false;
    m_resultFilePath = std::move(resultFilePath);
    m_finishedAt = finishedAt;
    m_errorMessage.clear();
    m_technicalDetails.clear();
    return true;
}

bool ProcessingTask::markFailed(QString errorMessage, QString technicalDetails, const QDateTime &finishedAt)
{
    if (isTerminal() || errorMessage.trimmed().isEmpty() || !finishedAt.isValid()) {
        return false;
    }

    m_status = ProcessingTaskStatus::Failed;
    m_errorMessage = std::move(errorMessage);
    m_technicalDetails = std::move(technicalDetails);
    m_finishedAt = finishedAt;
    m_resultFilePath.clear();
    return true;
}

bool ProcessingTask::cancel(const QDateTime &finishedAt)
{
    if (!canCancel() || !finishedAt.isValid()) {
        return false;
    }

    m_status = ProcessingTaskStatus::Canceled;
    m_finishedAt = finishedAt;
    m_errorMessage.clear();
    m_technicalDetails.clear();
    m_resultFilePath.clear();
    return true;
}

} // namespace AudioPlayer::Model
