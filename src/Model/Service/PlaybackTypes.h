#pragma once

#include <QMetaType>
#include <QString>

namespace AudioPlayer::Model::Service {

enum class PlaybackBackendState {
    Stopped,
    Loading,
    Playing,
    Paused,
    Buffering,
    Ended,
    Error,
};

enum class PlaybackBackendError {
    None,
    Resource,
    Format,
    Network,
    AccessDenied,
    Unknown,
};

struct PlaybackBackendErrorInfo
{
    PlaybackBackendError code = PlaybackBackendError::None;
    QString message;
};

} // namespace AudioPlayer::Model::Service

Q_DECLARE_METATYPE(AudioPlayer::Model::Service::PlaybackBackendState)
Q_DECLARE_METATYPE(AudioPlayer::Model::Service::PlaybackBackendError)
Q_DECLARE_METATYPE(AudioPlayer::Model::Service::PlaybackBackendErrorInfo)
