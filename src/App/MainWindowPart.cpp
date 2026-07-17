#include "App/MainWindowPart.h"

#include "Model/Service/ProcessingPlaylistIntegrationService.h"
#include "View/MainWindow.h"
#include "ViewModel/AudioEffectsViewModel.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackState.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"
#include "ViewModel/ProcessingViewModel.h"
#include "ViewModel/ShellViewModel.h"

#include <QStringList>

namespace AudioPlayer::App {
namespace {

QString playbackStateLabel(ViewModel::PlaybackState state)
{
    switch (state) {
    case ViewModel::PlaybackState::Loading:
        return QStringLiteral("Loading");
    case ViewModel::PlaybackState::Playing:
        return QStringLiteral("Playing");
    case ViewModel::PlaybackState::Paused:
        return QStringLiteral("Paused");
    case ViewModel::PlaybackState::Buffering:
        return QStringLiteral("Buffering");
    case ViewModel::PlaybackState::Error:
        return QStringLiteral("Error");
    case ViewModel::PlaybackState::Stopped:
        return QStringLiteral("Stopped");
    }
    return QStringLiteral("Stopped");
}

std::array<double, 5> equalizerGains(ViewModel::AudioEffectsViewModel &viewModel)
{
    return {viewModel.band0GainDb(),
            viewModel.band1GainDb(),
            viewModel.band2GainDb(),
            viewModel.band3GainDb(),
            viewModel.band4GainDb()};
}

} // namespace

MainWindowPart::MainWindowPart(View::MainWindow &view,
                               ViewModel::LibraryViewModel &libraryViewModel,
                               ViewModel::PlaylistCollectionViewModel &playlistViewModel,
                               ViewModel::PlaybackViewModel &playbackViewModel,
                               ViewModel::AudioEffectsViewModel &audioEffectsViewModel,
                               ViewModel::ProcessingViewModel &processingViewModel,
                               ViewModel::ShellViewModel &shellViewModel,
                               Model::Service::ProcessingPlaylistIntegrationService &processingIntegrationService,
                               QObject *parent)
    : QObject(parent)
    , m_view(view)
    , m_libraryViewModel(libraryViewModel)
    , m_playlistViewModel(playlistViewModel)
    , m_playbackViewModel(playbackViewModel)
    , m_audioEffectsViewModel(audioEffectsViewModel)
    , m_processingViewModel(processingViewModel)
    , m_shellViewModel(shellViewModel)
    , m_processingIntegrationService(processingIntegrationService)
{
    bindModelsAndCommands();
    bindLibrary();
    bindPlaylist();
    bindPlayback();
    bindAudioEffects();
    bindProcessing();
    syncAll();
}

void MainWindowPart::bindModelsAndCommands()
{
    m_view.setSongModel(m_libraryViewModel.songs());
    m_view.setPlaylistIdRole(m_playlistViewModel.playlistIdRole());
    m_view.setPlaylistModel(m_playlistViewModel.playlists());
    m_view.setProcessingTaskModel(m_processingViewModel.tasks());

    m_view.setLibraryCommands({m_libraryViewModel.scanCommand(), m_libraryViewModel.refreshCommand()});
    m_view.setPlaylistCommands({m_playlistViewModel.loadCommand(),
                                m_playlistViewModel.saveCommand(),
                                m_playlistViewModel.createPlaylistCommand(),
                                m_playlistViewModel.deletePlaylistCommand(),
                                m_playlistViewModel.switchPlaylistCommand(),
                                m_playlistViewModel.addSongsCommand(),
                                m_playlistViewModel.playSelectedSongCommand(),
                                m_playlistViewModel.playVisiblePlaylistCommand()});
    m_view.setPlaybackCommands({m_playbackViewModel.playCommand(),
                                m_playbackViewModel.pauseCommand(),
                                m_playbackViewModel.stopCommand(),
                                m_playbackViewModel.previousCommand(),
                                m_playbackViewModel.nextCommand(),
                                m_playbackViewModel.toggleMuteCommand(),
                                m_shellViewModel.pauseResumeCommand()});
    m_view.setAudioEffectsCommands({m_audioEffectsViewModel.resetPlaybackRateCommand(),
                                    m_audioEffectsViewModel.resetEqualizerCommand()});
    m_view.setProcessingCommands({m_processingViewModel.enqueueCommand(),
                                  m_processingViewModel.cancelSelectedCommand(),
                                  m_processingViewModel.cancelAllCommand()});
}

void MainWindowPart::bindLibrary()
{
    connect(&m_view, &View::MainWindow::scanPathEdited, &m_libraryViewModel, &ViewModel::LibraryViewModel::setScanDirectoryPath);
    connect(&m_view, &View::MainWindow::storagePathEdited, &m_libraryViewModel, &ViewModel::LibraryViewModel::setStoragePath);
    connect(&m_libraryViewModel, &ViewModel::LibraryViewModel::scanDirectoryPathChanged, this, [this]() {
        m_view.setScanDirectoryPath(m_libraryViewModel.scanDirectoryPath());
    });
    connect(&m_libraryViewModel, &ViewModel::LibraryViewModel::countChanged, this, [this]() {
        m_view.setLibraryCount(m_libraryViewModel.count());
    });
    connect(&m_libraryViewModel, &ViewModel::LibraryViewModel::statusMessageChanged, this, [this]() {
        m_view.setLibraryStatusMessage(m_libraryViewModel.statusMessage());
    });
    connect(&m_libraryViewModel, &ViewModel::LibraryViewModel::lastErrorChanged, this, [this]() {
        m_view.setLibraryLastError(m_libraryViewModel.lastError());
    });
    connect(&m_libraryViewModel, &ViewModel::LibraryViewModel::warningsChanged, this, [this]() {
        m_view.setLibraryWarnings(m_libraryViewModel.warnings());
    });
}

void MainWindowPart::bindPlaylist()
{
    connect(&m_view, &View::MainWindow::storagePathEdited, &m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::setStoragePath);
    connect(&m_view, &View::MainWindow::playlistNameEdited, &m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::setNewPlaylistName);
    connect(&m_view, &View::MainWindow::playlistSelected, &m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::setSelectedPlaylistId);
    connect(&m_view, &View::MainWindow::playlistSelected, m_playlistViewModel.switchPlaylistCommand(), &Common::ViewCommand::execute);
    connect(&m_view, &View::MainWindow::songSelected, &m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::setSelectedSongIndex);
    connect(&m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::storagePathChanged, this, [this]() {
        m_view.setStoragePath(m_playlistViewModel.storagePath());
        m_processingIntegrationService.setStoragePath(m_playlistViewModel.storagePath());
    });
    connect(&m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::newPlaylistNameChanged, this, [this]() {
        m_view.setPlaylistName(m_playlistViewModel.newPlaylistName());
    });
    connect(&m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::visiblePlaylistChanged, this, [this]() {
        m_view.setVisiblePlaylistId(m_playlistViewModel.visiblePlaylistId());
    });
    connect(&m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::statusMessageChanged, this, [this]() {
        m_view.setPlaylistStatusMessage(m_playlistViewModel.statusMessage());
    });
    connect(&m_playlistViewModel, &ViewModel::PlaylistCollectionViewModel::lastErrorChanged, this, [this]() {
        m_view.setPlaylistLastError(m_playlistViewModel.lastError());
    });
}

void MainWindowPart::bindPlayback()
{
    connect(&m_view, &View::MainWindow::seekRequested, &m_playbackViewModel, &ViewModel::PlaybackViewModel::seekTo);
    connect(&m_view, &View::MainWindow::volumeChangedByUser, &m_playbackViewModel, &ViewModel::PlaybackViewModel::setVolumePercent);
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::playbackStateChanged, this, [this]() {
        m_view.setPlaybackStateLabel(playbackStateLabel(m_playbackViewModel.playbackState()));
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::currentPlaybackTitleChanged, this, [this]() {
        m_view.setPlaybackHasCurrentTrack(m_playbackViewModel.currentPlaybackIndex() >= 0);
        m_view.setCurrentPlaybackTitle(m_playbackViewModel.currentPlaybackTitle());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::currentPlaybackIndexChanged, this, [this]() {
        m_view.setPlaybackHasCurrentTrack(m_playbackViewModel.currentPlaybackIndex() >= 0);
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::positionMsChanged, this, [this]() {
        m_view.setPlaybackPosition(m_playbackViewModel.positionMs());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::durationMsChanged, this, [this]() {
        m_view.setPlaybackDuration(m_playbackViewModel.durationMs());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::seekableChanged, this, [this]() {
        m_view.setPlaybackSeekable(m_playbackViewModel.seekable());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::volumePercentChanged, this, [this]() {
        m_view.setPlaybackVolumePercent(m_playbackViewModel.volumePercent());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::mutedChanged, this, [this]() {
        m_view.setPlaybackMuted(m_playbackViewModel.muted());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::lastPlaybackErrorChanged, this, [this]() {
        m_view.setPlaybackLastError(m_playbackViewModel.lastPlaybackError());
    });
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModel::statusMessageChanged, this, [this]() {
        m_view.setPlaybackStatusMessage(m_playbackViewModel.statusMessage());
    });
}

void MainWindowPart::bindAudioEffects()
{
    connect(&m_view, &View::MainWindow::playbackRateSelected, &m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::setPlaybackRate);
    connect(&m_view, &View::MainWindow::equalizerEnabledChangedByUser, &m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::setEqualizerEnabled);
    connect(&m_view, &View::MainWindow::equalizerBandGainChanged, &m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::setEqualizerBandGain);
    connect(&m_view, &View::MainWindow::equalizerPresetSelected, &m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::setEqualizerPreset);
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::capabilitiesChanged, this, [this]() {
        m_view.setAudioEffectsCapabilities(m_audioEffectsViewModel.supportsPitchPreservingRate(),
                                           m_audioEffectsViewModel.supportsEqualizer());
    });
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::playbackRateChanged, this, [this]() {
        m_view.setAudioEffectsPlaybackRate(m_audioEffectsViewModel.playbackRate());
    });
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::equalizerEnabledChanged, this, [this]() {
        m_view.setAudioEffectsEqualizerEnabled(m_audioEffectsViewModel.equalizerEnabled());
    });
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::equalizerBandGainsChanged, this, [this]() {
        m_view.setAudioEffectsBandGains(equalizerGains(m_audioEffectsViewModel));
    });
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::equalizerPresetChanged, this, [this]() {
        m_view.setAudioEffectsPreset(m_audioEffectsViewModel.equalizerPreset());
    });
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::statusMessageChanged, this, [this]() {
        m_view.setAudioEffectsStatusMessage(m_audioEffectsViewModel.statusMessage());
    });
    connect(&m_audioEffectsViewModel, &ViewModel::AudioEffectsViewModel::lastErrorChanged, this, [this]() {
        m_view.setAudioEffectsLastError(m_audioEffectsViewModel.lastError());
    });
}

void MainWindowPart::bindProcessing()
{
    connect(&m_view, &View::MainWindow::processingInputsSelected, &m_processingViewModel, &ViewModel::ProcessingViewModel::setInputFilePaths);
    connect(&m_view, &View::MainWindow::processingOutputDirectorySelected, &m_processingViewModel, &ViewModel::ProcessingViewModel::setOutputDirectory);
    connect(&m_view, &View::MainWindow::processingFormatSelected, &m_processingViewModel, &ViewModel::ProcessingViewModel::setOutputFormat);
    connect(&m_view, &View::MainWindow::processingTaskSelected, &m_processingViewModel, &ViewModel::ProcessingViewModel::setSelectedTaskRow);
    connect(&m_processingViewModel, &ViewModel::ProcessingViewModel::inputFilePathsChanged, this, [this]() {
        m_view.setProcessingInputFilePaths(m_processingViewModel.inputFilePaths());
    });
    connect(&m_processingViewModel, &ViewModel::ProcessingViewModel::outputDirectoryChanged, this, [this]() {
        m_view.setProcessingOutputDirectory(m_processingViewModel.outputDirectory());
    });
    connect(&m_processingViewModel, &ViewModel::ProcessingViewModel::outputFormatChanged, this, [this]() {
        m_view.setProcessingOutputFormat(m_processingViewModel.outputFormat());
    });
    connect(&m_processingViewModel, &ViewModel::ProcessingViewModel::hasPendingOrRunningTasksChanged, this, [this]() {
        m_view.setProcessingHasPendingOrRunningTasks(m_processingViewModel.hasPendingOrRunningTasks());
    });
    connect(&m_processingViewModel, &ViewModel::ProcessingViewModel::statusMessageChanged, this, [this]() {
        m_view.setProcessingStatusMessage(m_processingViewModel.statusMessage());
    });
    connect(&m_processingViewModel, &ViewModel::ProcessingViewModel::lastErrorChanged, this, [this]() {
        m_view.setProcessingLastError(m_processingViewModel.lastError());
    });
    connect(&m_processingIntegrationService, &Model::Service::ProcessingPlaylistIntegrationService::integrationWarning,
            &m_processingViewModel, &ViewModel::ProcessingViewModel::reportIntegrationWarning);
}

void MainWindowPart::syncAll()
{
    m_view.setStoragePath(m_playlistViewModel.storagePath());
    m_libraryViewModel.setStoragePath(m_playlistViewModel.storagePath());
    m_processingIntegrationService.setStoragePath(m_playlistViewModel.storagePath());
    m_view.setScanDirectoryPath(m_libraryViewModel.scanDirectoryPath());
    m_view.setLibraryCount(m_libraryViewModel.count());
    m_view.setLibraryStatusMessage(m_libraryViewModel.statusMessage());
    m_view.setLibraryLastError(m_libraryViewModel.lastError());
    m_view.setLibraryWarnings(m_libraryViewModel.warnings());
    m_view.setPlaylistName(m_playlistViewModel.newPlaylistName());
    m_view.setVisiblePlaylistId(m_playlistViewModel.visiblePlaylistId());
    m_view.setPlaylistStatusMessage(m_playlistViewModel.statusMessage());
    m_view.setPlaylistLastError(m_playlistViewModel.lastError());
    m_view.setPlaybackStateLabel(playbackStateLabel(m_playbackViewModel.playbackState()));
    m_view.setPlaybackHasCurrentTrack(m_playbackViewModel.currentPlaybackIndex() >= 0);
    m_view.setCurrentPlaybackTitle(m_playbackViewModel.currentPlaybackTitle());
    m_view.setPlaybackPosition(m_playbackViewModel.positionMs());
    m_view.setPlaybackDuration(m_playbackViewModel.durationMs());
    m_view.setPlaybackSeekable(m_playbackViewModel.seekable());
    m_view.setPlaybackVolumePercent(m_playbackViewModel.volumePercent());
    m_view.setPlaybackMuted(m_playbackViewModel.muted());
    m_view.setPlaybackLastError(m_playbackViewModel.lastPlaybackError());
    m_view.setPlaybackStatusMessage(m_playbackViewModel.statusMessage());
    m_view.setPlaybackRateLabels(m_audioEffectsViewModel.playbackRateLabels());
    m_view.setEqualizerPresetLabels(m_audioEffectsViewModel.equalizerPresetLabels());
    m_view.setAudioEffectsCapabilities(m_audioEffectsViewModel.supportsPitchPreservingRate(),
                                       m_audioEffectsViewModel.supportsEqualizer());
    m_view.setAudioEffectsPlaybackRate(m_audioEffectsViewModel.playbackRate());
    m_view.setAudioEffectsEqualizerEnabled(m_audioEffectsViewModel.equalizerEnabled());
    m_view.setAudioEffectsBandGains(equalizerGains(m_audioEffectsViewModel));
    m_view.setAudioEffectsPreset(m_audioEffectsViewModel.equalizerPreset());
    m_view.setAudioEffectsStatusMessage(m_audioEffectsViewModel.statusMessage());
    m_view.setAudioEffectsLastError(m_audioEffectsViewModel.lastError());
    m_view.setProcessingInputFilePaths(m_processingViewModel.inputFilePaths());
    m_view.setProcessingOutputDirectory(m_processingViewModel.outputDirectory());
    m_view.setProcessingOutputFormat(m_processingViewModel.outputFormat());
    m_view.setProcessingHasPendingOrRunningTasks(m_processingViewModel.hasPendingOrRunningTasks());
    m_view.setProcessingStatusMessage(m_processingViewModel.statusMessage());
    m_view.setProcessingLastError(m_processingViewModel.lastError());
}

} // namespace AudioPlayer::App
