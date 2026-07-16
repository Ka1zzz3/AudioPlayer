#pragma once

#include <QString>

namespace AudioPlayer::Model {

enum class ProcessingTaskType
{
    Transcode,
};

enum class ProcessingTaskStatus
{
    Pending,
    Running,
    Succeeded,
    Failed,
    Canceled,
};

enum class ProcessingOutputFormat
{
    Mp3,
    Wav,
    Flac,
};

[[nodiscard]] QString toString(ProcessingTaskType type);
[[nodiscard]] QString toString(ProcessingTaskStatus status);
[[nodiscard]] QString toString(ProcessingOutputFormat format);
[[nodiscard]] bool isTerminal(ProcessingTaskStatus status) noexcept;

} // namespace AudioPlayer::Model
