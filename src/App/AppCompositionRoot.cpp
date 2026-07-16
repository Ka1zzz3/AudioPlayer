#include "App/AppCompositionRoot.h"

#include "Model/Service/LibraryUseCase.h"
#include "Model/Service/QtMultimediaPlaybackService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "View/MainWindow.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackViewModel.h"
#include "ViewModel/PlaylistCollectionViewModel.h"

#include <QApplication>

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

    View::MainWindow mainWindow(libraryViewModel, playlistCollectionViewModel, playbackViewModel);

    mainWindow.show();
    return application.exec();
}

} // namespace AudioPlayer::App
