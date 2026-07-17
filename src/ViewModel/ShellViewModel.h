#pragma once

#include "Common/ViewCommand.h"
#include "ViewModel/PlaybackState.h"

#include <QObject>

namespace AudioPlayer::ViewModel {

class LibraryViewModel;
class PlaybackViewModel;
class PlaylistCollectionViewModel;

class ShellViewModel : public QObject
{
    Q_OBJECT

public:
    ShellViewModel(LibraryViewModel &libraryViewModel,
                   PlaylistCollectionViewModel &playlistViewModel,
                   PlaybackViewModel &playbackViewModel,
                   QObject *parent = nullptr);

    [[nodiscard]] Common::ViewCommand *pauseResumeCommand() noexcept;

private:
    bool executePauseResume();

    LibraryViewModel &m_libraryViewModel;
    PlaylistCollectionViewModel &m_playlistViewModel;
    PlaybackViewModel &m_playbackViewModel;
    Common::ViewCommand m_pauseResumeCommand;
};

} // namespace AudioPlayer::ViewModel
