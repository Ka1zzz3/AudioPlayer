#include "App/AppCompositionRoot.h"

#include "Model/Service/LibraryUseCase.h"
#include "Model/Service/NullPlaybackService.h"
#include "Model/Service/PlaybackUseCase.h"
#include "View/MainWindow.h"
#include "ViewModel/LibraryViewModel.h"
#include "ViewModel/PlaybackViewModel.h"

#include <QApplication>

#include <memory>

namespace AudioPlayer::App {

int runWidgetsApplication(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto libraryUseCase = std::make_shared<const Model::Service::LibraryUseCase>();
    ViewModel::LibraryViewModel libraryViewModel(std::move(libraryUseCase));

    Model::Service::NullPlaybackService playbackService;
    Model::Service::PlaybackUseCase playbackUseCase(playbackService);
    ViewModel::PlaybackViewModel playbackViewModel(playbackUseCase, playbackService);

    const auto syncPlaybackQueue = [&libraryViewModel, &playbackViewModel]() {
        playbackViewModel.replaceQueue(libraryViewModel.audioFilesSnapshot());
    };
    QObject::connect(&libraryViewModel, &ViewModel::LibraryViewModel::libraryChanged, &playbackViewModel, syncPlaybackQueue);
    syncPlaybackQueue();

    View::MainWindow mainWindow(libraryViewModel, playbackViewModel);

    mainWindow.show();
    return application.exec();
}

} // namespace AudioPlayer::App
