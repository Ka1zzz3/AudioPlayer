#include "App/AppCompositionRoot.h"

#include "Model/Service/LibraryUseCase.h"
#include "View/MainWindow.h"
#include "ViewModel/LibraryViewModel.h"

#include <QApplication>

#include <memory>

namespace AudioPlayer::App {

int runWidgetsApplication(int argc, char *argv[])
{
    QApplication application(argc, argv);

    auto libraryUseCase = std::make_shared<const Model::Service::LibraryUseCase>();
    ViewModel::LibraryViewModel libraryViewModel(std::move(libraryUseCase));
    View::MainWindow mainWindow(libraryViewModel);

    mainWindow.show();
    return application.exec();
}

} // namespace AudioPlayer::App
