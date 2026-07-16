#include "App/AppCompositionRoot.h"

#include "Model/ProcessingTask.h"
#include "Model/Service/LibraryUseCase.h"
#include "Model/Service/FfmpegTranscodingBackend.h"
#include "Model/Service/QtMultimediaPlaybackService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "Model/Service/ProcessingUseCase.h"
#include "Model/Service/TranscodedPlaylistService.h"
#include "View/MainWindow.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"
#include "ViewModel/ProcessingViewModel.h"

#include <QApplication>
#include <QFileInfo>
#include <QSet>

#include <memory>

namespace AudioPlayer::App {

int runWidgetsApplication(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto libraryUseCase = std::make_shared<const Model::Service::LibraryUseCase>();
    ViewModel::LibraryViewModel libraryViewModel(std::move(libraryUseCase));

    Model::Service::QtMultimediaPlaybackService playbackService;
    Model::Service::PlaybackUseCase playbackUseCase(playbackService);
    ViewModel::PlaybackViewModel playbackViewModel(playbackUseCase, playbackService);
    ViewModel::PlaylistCollectionViewModel playlistCollectionViewModel;
    Model::Service::FfmpegTranscodingBackend transcodingBackend;
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
