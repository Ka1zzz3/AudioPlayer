#pragma once

#include "Model/ProcessingTypes.h"

#include <QDateTime>
#include <QString>

namespace AudioPlayer::Model {

class ProcessingTask
{
public:
    ProcessingTask() = default;
    ProcessingTask(QString id,
                   ProcessingTaskType type,
                   QString inputFilePath,
                   QString outputFilePath,
                   ProcessingOutputFormat outputFormat,
                   QDateTime createdAt);

    [[nodiscard]] const QString &id() const noexcept;
    [[nodiscard]] ProcessingTaskType type() const noexcept;
    [[nodiscard]] const QString &inputFilePath() const noexcept;
    [[nodiscard]] const QString &outputFilePath() const noexcept;
    [[nodiscard]] ProcessingOutputFormat outputFormat() const noexcept;
    [[nodiscard]] ProcessingTaskStatus status() const noexcept;
    [[nodiscard]] int progressPercent() const noexcept;
    [[nodiscard]] bool progressIndeterminate() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] const QString &technicalDetails() const noexcept;
    [[nodiscard]] const QString &resultFilePath() const noexcept;
    [[nodiscard]] const QDateTime &createdAt() const noexcept;
    [[nodiscard]] const QDateTime &startedAt() const noexcept;
    [[nodiscard]] const QDateTime &finishedAt() const noexcept;

    [[nodiscard]] bool isValid() const noexcept;
    [[nodiscard]] bool canCancel() const noexcept;
    [[nodiscard]] bool isTerminal() const noexcept;

    bool markRunning(const QDateTime &startedAt);
    bool setProgressPercent(int percent) noexcept;
    bool setProgressIndeterminate() noexcept;
    bool markSucceeded(QString resultFilePath, const QDateTime &finishedAt);
    bool markFailed(QString errorMessage, QString technicalDetails, const QDateTime &finishedAt);
    bool cancel(const QDateTime &finishedAt);

private:
    QString m_id;
    ProcessingTaskType m_type = ProcessingTaskType::Transcode;
    QString m_inputFilePath;
    QString m_outputFilePath;
    ProcessingOutputFormat m_outputFormat = ProcessingOutputFormat::Mp3;
    ProcessingTaskStatus m_status = ProcessingTaskStatus::Pending;
    int m_progressPercent = 0;
    bool m_progressIndeterminate = false;
    QString m_errorMessage;
    QString m_technicalDetails;
    QString m_resultFilePath;
    QDateTime m_createdAt;
    QDateTime m_startedAt;
    QDateTime m_finishedAt;
};

} // namespace AudioPlayer::Model
