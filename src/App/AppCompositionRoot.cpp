#include "App/AppCompositionRoot.h"

#include "App/MainWindowPart.h"
#include "Model/Service/AudioEffectsUseCase.h"
#include "Model/Service/FfmpegTranscodingBackend.h"
#include "Model/Service/IAudioEffectsService.h"
#include "Model/Service/IPlaybackService.h"
#include "Model/Service/LibraryUseCase.h"
#include "Model/Service/NullAudioEffectsService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "Model/Service/PlaylistCollectionUseCase.h"
#include "Model/Service/ProcessingPlaylistIntegrationService.h"
#include "Model/Service/ProcessingUseCase.h"
#include "Model/Service/QtMultimediaPlaybackService.h"
#include "Model/Service/TranscodedPlaylistService.h"
#include "View/MainWindow.h"
#include "ViewModel/AudioEffectsViewModel.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"
#include "ViewModel/ProcessingViewModel.h"
#include "ViewModel/ShellViewModel.h"

#include <QApplication>

#include <memory>

#if AUDIOPLAYER_GSTREAMER_EFFECTS_AVAILABLE
#include "Model/Service/GStreamerEffectsPlaybackBackend.h"
#endif

namespace AudioPlayer::App {

namespace {

#ifndef AUDIOPLAYER_FFMPEG_EXECUTABLE
#define AUDIOPLAYER_FFMPEG_EXECUTABLE "ffmpeg"
#endif

#ifndef AUDIOPLAYER_FFPROBE_EXECUTABLE
#define AUDIOPLAYER_FFPROBE_EXECUTABLE "ffprobe"
#endif

} // namespace

int runWidgetsApplication(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto libraryUseCase = std::make_shared<const Model::Service::LibraryUseCase>();
    ViewModel::LibraryViewModel libraryViewModel(std::move(libraryUseCase));

    auto playlistUseCase = std::make_shared<const Model::Service::PlaylistCollectionUseCase>();
    ViewModel::PlaylistCollectionViewModel playlistCollectionViewModel(std::move(playlistUseCase));

    std::unique_ptr<Model::Service::IPlaybackService> playbackService;
    std::unique_ptr<Model::Service::IAudioEffectsService> audioEffectsService;

#if AUDIOPLAYER_GSTREAMER_EFFECTS_AVAILABLE
    auto gstreamerEffectsCore = std::make_shared<Model::Service::GStreamerEffectsBackendCore>();
    playbackService = std::make_unique<Model::Service::GStreamerPlaybackService>(gstreamerEffectsCore);
    audioEffectsService = std::make_unique<Model::Service::GStreamerAudioEffectsService>(gstreamerEffectsCore);
#else
    playbackService = std::make_unique<Model::Service::QtMultimediaPlaybackService>();
    audioEffectsService = std::make_unique<Model::Service::NullAudioEffectsService>(
        QStringLiteral("GStreamer effects backend is unavailable; ordinary Qt Multimedia playback fallback is active."));
#endif

    Model::Service::PlaybackUseCase playbackUseCase(*playbackService);
    ViewModel::PlaybackViewModel playbackViewModel(playbackUseCase);

    auto audioEffectsUseCase = std::make_shared<Model::Service::AudioEffectsUseCase>(audioEffectsService.get());
    ViewModel::AudioEffectsViewModel audioEffectsViewModel(audioEffectsUseCase);

    Model::Service::FfmpegTranscodingBackend transcodingBackend(QString::fromUtf8(AUDIOPLAYER_FFMPEG_EXECUTABLE),
                                                                QString::fromUtf8(AUDIOPLAYER_FFPROBE_EXECUTABLE));
    auto processingUseCase = std::make_shared<Model::Service::ProcessingUseCase>(&transcodingBackend);
    ViewModel::ProcessingViewModel processingViewModel(processingUseCase);

    Model::Service::TranscodedPlaylistService transcodedPlaylistService(QStringLiteral("storage/library.json"));
    Model::Service::ProcessingPlaylistIntegrationService processingIntegrationService(*processingUseCase,
                                                                                      transcodedPlaylistService);

    ViewModel::ShellViewModel shellViewModel(libraryViewModel, playlistCollectionViewModel, playbackViewModel);
    View::MainWindow mainWindow;
    MainWindowPart mainWindowPart(mainWindow,
                                  libraryViewModel,
                                  playlistCollectionViewModel,
                                  playbackViewModel,
                                  audioEffectsViewModel,
                                  processingViewModel,
                                  shellViewModel,
                                  processingIntegrationService);

    mainWindow.show();
    return application.exec();
}

} // namespace AudioPlayer::App
