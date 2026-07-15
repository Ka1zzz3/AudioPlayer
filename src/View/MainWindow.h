#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

class QLabel;
class QLineEdit;
class QListView;
class QPushButton;
class QWidget;

namespace AudioPlayer::Common {
class ViewCommand;
}

namespace AudioPlayer::ViewModel {
class LibraryViewModelProtocol;
class PlaybackViewModelProtocol;
}

namespace AudioPlayer::View {

class MainWindow : public QMainWindow
{
public:
    MainWindow(ViewModel::LibraryViewModelProtocol &libraryViewModel,
               ViewModel::PlaybackViewModelProtocol &playbackViewModel,
               QWidget *parent = nullptr);

private:
    void buildUi();
    void bindViewModel();
    void bindLineEdit(QLineEdit &lineEdit,
                      const QString &(ViewModel::LibraryViewModelProtocol::*getter)() const noexcept,
                      void (ViewModel::LibraryViewModelProtocol::*setter)(QString),
                      void (ViewModel::LibraryViewModelProtocol::*changedSignal)());
    void bindButton(QPushButton &button, Common::ViewCommand *command);
    void updateCount();
    void updateStatusMessage();
    void updateLastError();
    void updateWarnings();
    void setLabelVisibleText(QLabel &label, const QString &text);

    ViewModel::LibraryViewModelProtocol &m_viewModel;
    ViewModel::PlaybackViewModelProtocol &m_playbackViewModel;
    QLineEdit *m_storagePathInput = nullptr;
    QLineEdit *m_scanDirectoryPathInput = nullptr;
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_loadButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QLabel *m_warningsLabel = nullptr;
    QListView *m_songListView = nullptr;
};

} // namespace AudioPlayer::View
