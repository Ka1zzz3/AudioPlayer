#include "Model/ProcessingTypes.h"

namespace AudioPlayer::Model {

QString toString(ProcessingTaskType type)
{
    switch (type) {
    case ProcessingTaskType::Transcode:
        return QStringLiteral("transcode");
    }
    return QStringLiteral("unknown");
}

QString toString(ProcessingTaskStatus status)
{
    switch (status) {
    case ProcessingTaskStatus::Pending:
        return QStringLiteral("pending");
    case ProcessingTaskStatus::Running:
        return QStringLiteral("running");
    case ProcessingTaskStatus::Succeeded:
        return QStringLiteral("succeeded");
    case ProcessingTaskStatus::Failed:
        return QStringLiteral("failed");
    case ProcessingTaskStatus::Canceled:
        return QStringLiteral("canceled");
    }
    return QStringLiteral("unknown");
}

QString toString(ProcessingOutputFormat format)
{
    switch (format) {
    case ProcessingOutputFormat::Mp3:
        return QStringLiteral("mp3");
    case ProcessingOutputFormat::Wav:
        return QStringLiteral("wav");
    case ProcessingOutputFormat::Flac:
        return QStringLiteral("flac");
    }
    return QStringLiteral("unknown");
}

bool isTerminal(ProcessingTaskStatus status) noexcept
{
    return status == ProcessingTaskStatus::Succeeded
        || status == ProcessingTaskStatus::Failed
        || status == ProcessingTaskStatus::Canceled;
}

} // namespace AudioPlayer::Model
