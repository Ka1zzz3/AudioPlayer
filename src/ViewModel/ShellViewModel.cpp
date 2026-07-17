#include "ViewModel/ShellViewModel.h"

#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"

namespace AudioPlayer::ViewModel {

ShellViewModel::ShellViewModel(LibraryViewModel &libraryViewModel,
                               PlaylistCollectionViewModel &playlistViewModel,
                               PlaybackViewModel &playbackViewModel,
                               QObject *parent)
    : QObject(parent)
    , m_libraryViewModel(libraryViewModel)
    , m_playlistViewModel(playlistViewModel)
    , m_playbackViewModel(playbackViewModel)
    , m_pauseResumeCommand(QStringLiteral("pauseResume"), [this]() { return executePauseResume(); }, this)
{
    connect(&m_libraryViewModel, &LibraryViewModel::libraryChanged, this, [this]() {
        m_playlistViewModel.setCurrentVisibleSongs(m_libraryViewModel.audioFilesSnapshot());
    });

    connect(&m_playlistViewModel,
            &PlaylistCollectionViewModel::currentVisibleSongsChanged,
            this,
            [this](const QVector<Model::AudioFile> &songs) {
                m_libraryViewModel.setVisibleSongsProjection(songs);
            });

    connect(&m_playlistViewModel,
            &PlaylistCollectionViewModel::playRequested,
            this,
            [this](const QString &, const QVector<Model::AudioFile> &songsSnapshot, int startIndex) {
                m_playbackViewModel.replaceQueue(songsSnapshot);
                m_playbackViewModel.setCurrentPlaybackIndex(startIndex);
                m_playbackViewModel.playCommand()->execute();
            });
}

Common::ViewCommand *ShellViewModel::pauseResumeCommand() noexcept
{
    return &m_pauseResumeCommand;
}

bool ShellViewModel::executePauseResume()
{
    if (m_playbackViewModel.playbackState() == PlaybackState::Playing) {
        return m_playbackViewModel.pauseCommand()->execute();
    }
    return m_playbackViewModel.playCommand()->execute();
}

} // namespace AudioPlayer::ViewModel
