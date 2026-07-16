#include "App/AppCompositionRoot.h"

#include "Model/ProcessingTask.h"
#include "Model/Service/LibraryUseCase.h"
#include "Model/Service/AudioEffectsUseCase.h"
#include "Model/Service/FfmpegTranscodingBackend.h"
#include "Model/Service/IAudioEffectsService.h"
#include "Model/Service/IPlaybackService.h"
#include "Model/Service/NullAudioEffectsService.h"
#include "Model/Service/QtMultimediaPlaybackService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "Model/Service/ProcessingUseCase.h"
#include "Model/Service/TranscodedPlaylistService.h"
#include "View/MainWindow.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/AudioEffectsViewModel.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"
#include "ViewModel/ProcessingViewModel.h"

#include <QApplication>
#include <QFileInfo>
#include <QSet>

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
    ViewModel::PlaybackViewModel playbackViewModel(playbackUseCase, *playbackService);
    auto audioEffectsUseCase = std::make_shared<Model::Service::AudioEffectsUseCase>(audioEffectsService.get());
    ViewModel::AudioEffectsViewModel audioEffectsViewModel(audioEffectsUseCase);
    ViewModel::PlaylistCollectionViewModel playlistCollectionViewModel;
    Model::Service::FfmpegTranscodingBackend transcodingBackend(QString::fromUtf8(AUDIOPLAYER_FFMPEG_EXECUTABLE),
                                                                QString::fromUtf8(AUDIOPLAYER_FFPROBE_EXECUTABLE));
    auto processingUseCase = std::make_shared<Model::Service::ProcessingUseCase>(&transcodingBackend);
    ViewModel::ProcessingViewModel processingViewModel(processingUseCase);
    Model::Service::TranscodedPlaylistService transcodedPlaylistService(QStringLiteral("storage/library.json"));
    QSet<QString> playlistIntegratedTaskIds;

    QObject::connect(&libraryViewModel,
                     &ViewModel::LibraryViewModel::libraryChanged,
                     &playlistCollectionViewModel,
                     [&libraryViewModel, &playlistCollectionViewModel]() {
                         playlistCollectionViewModel.setCurrentVisibleSongs(libraryViewModel.audioFilesSnapshot());
                     });

    QObject::connect(&playlistCollectionViewModel,
                     &ViewModel::PlaylistCollectionViewModel::currentVisibleSongsChanged,
                     &libraryViewModel,
                     [&libraryViewModel](const QVector<Model::AudioFile> &songs) {
                         libraryViewModel.setVisibleSongsProjection(songs);
                     });

    QObject::connect(&playlistCollectionViewModel,
                     &ViewModel::PlaylistCollectionViewModel::playRequested,
                     &playbackViewModel,
                     [&playbackViewModel](const QString &,
                                          const QVector<Model::AudioFile> &songsSnapshot,
                                          int startIndex) {
                         playbackViewModel.replaceQueue(songsSnapshot);
                         playbackViewModel.setCurrentPlaybackIndex(startIndex);
                         playbackViewModel.playCommand()->execute();
                     });
    QObject::connect(&playlistCollectionViewModel,
                     &ViewModel::PlaylistCollectionViewModel::storagePathChanged,
                     &playlistCollectionViewModel,
                     [&playlistCollectionViewModel, &transcodedPlaylistService]() {
                         transcodedPlaylistService.setStoragePath(playlistCollectionViewModel.storagePath());
                     });

    QObject::connect(processingUseCase.get(),
                     &Model::Service::ProcessingUseCase::taskUpdated,
                     &processingViewModel,
                     [&processingUseCase,
                      &playlistIntegratedTaskIds,
                      &transcodedPlaylistService,
                      &processingViewModel](const QString &taskId) {
                         if (playlistIntegratedTaskIds.contains(taskId)) {
                             return;
                         }

                         const auto tasks = processingUseCase->tasks();
                         for (const auto &task : tasks) {
                             if (task.id() != taskId || task.status() != Model::ProcessingTaskStatus::Succeeded) {
                                 continue;
                             }

                             playlistIntegratedTaskIds.insert(taskId);
                             const QFileInfo outputInfo(task.resultFilePath());
                             const QString title = outputInfo.completeBaseName().isEmpty()
                                 ? outputInfo.fileName()
                                 : outputInfo.completeBaseName();
                             const auto result = transcodedPlaylistService.addOutput(
                                 Model::AudioFile(task.resultFilePath(), title),
                                 QDateTime::currentDateTimeUtc());
                             if (!result.ok()) {
                                 processingViewModel.reportIntegrationWarning(
                                     QStringLiteral("Transcode succeeded, but adding to Transcoded playlist failed: %1")
                                         .arg(result.error));
                             } else if (!result.status.isEmpty()) {
                                 processingViewModel.reportIntegrationWarning({});
                             }
                             return;
                         }
                     });

    View::MainWindow mainWindow(libraryViewModel, playlistCollectionViewModel, playbackViewModel, processingViewModel);

    mainWindow.show();
    return application.exec();
}

} // namespace AudioPlayer::App
