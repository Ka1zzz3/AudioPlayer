#pragma once

#include <QObject>
#include <array>

namespace AudioPlayer::Model::Service {
class ProcessingPlaylistIntegrationService;
}

namespace AudioPlayer::View {
class MainWindow;
}

namespace AudioPlayer::ViewModel {
class AudioEffectsViewModel;
class LibraryViewModel;
class PlaybackViewModel;
class PlaylistCollectionViewModel;
class ProcessingViewModel;
class ShellViewModel;
}

namespace AudioPlayer::App {

class MainWindowPart : public QObject
{
    Q_OBJECT

public:
    MainWindowPart(View::MainWindow &view,
                   ViewModel::LibraryViewModel &libraryViewModel,
                   ViewModel::PlaylistCollectionViewModel &playlistViewModel,
                   ViewModel::PlaybackViewModel &playbackViewModel,
                   ViewModel::AudioEffectsViewModel &audioEffectsViewModel,
                   ViewModel::ProcessingViewModel &processingViewModel,
                   ViewModel::ShellViewModel &shellViewModel,
                   Model::Service::ProcessingPlaylistIntegrationService &processingIntegrationService,
                   QObject *parent = nullptr);

private:
    void bindModelsAndCommands();
    void bindLibrary();
    void bindPlaylist();
    void bindPlayback();
    void bindAudioEffects();
    void bindProcessing();
    void syncAll();

    View::MainWindow &m_view;
    ViewModel::LibraryViewModel &m_libraryViewModel;
    ViewModel::PlaylistCollectionViewModel &m_playlistViewModel;
    ViewModel::PlaybackViewModel &m_playbackViewModel;
    ViewModel::AudioEffectsViewModel &m_audioEffectsViewModel;
    ViewModel::ProcessingViewModel &m_processingViewModel;
    ViewModel::ShellViewModel &m_shellViewModel;
    Model::Service::ProcessingPlaylistIntegrationService &m_processingIntegrationService;
};

} // namespace AudioPlayer::App
