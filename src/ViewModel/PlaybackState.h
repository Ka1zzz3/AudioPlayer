#pragma once

#include <QMetaType>

namespace AudioPlayer::ViewModel {

enum class PlaybackState {
    Stopped,
    Loading,
    Playing,
    Paused,
    Buffering,
    Error,
};

} // namespace AudioPlayer::ViewModel

Q_DECLARE_METATYPE(AudioPlayer::ViewModel::PlaybackState)
