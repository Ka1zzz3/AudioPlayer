#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

class QLabel;
class QLineEdit;
class QListView;
class QPushButton;
class QSlider;
class QWidget;

namespace AudioPlayer::Common {
class ViewCommand;
}

namespace AudioPlayer::ViewModel {
class LibraryViewModelProtocol;
class PlaybackViewModelProtocol;
class PlaylistCollectionViewModelProtocol;
enum class PlaybackState;
}

namespace AudioPlayer::View {

class MainWindow : public QMainWindow
{
public:
    MainWindow(ViewModel::LibraryViewModelProtocol &libraryViewModel,
               ViewModel::PlaylistCollectionViewModelProtocol &playlistViewModel,
               ViewModel::PlaybackViewModelProtocol &playbackViewModel,
               QWidget *parent = nullptr);

private:
    void buildUi();
    void bindViewModel();
    void bindLibraryViewModel();
    void bindPlaylistCollectionViewModel();
    void bindPlaybackViewModel();
    void bindLineEdit(QLineEdit &lineEdit,
                      const QString &(ViewModel::LibraryViewModelProtocol::*getter)() const noexcept,
                      void (ViewModel::LibraryViewModelProtocol::*setter)(QString),
                      void (ViewModel::LibraryViewModelProtocol::*changedSignal)());
    void bindButton(QPushButton &button, Common::ViewCommand *command);
    void updateCount();
    void updateStatusMessage();
    void updateLastError();
    void updateWarnings();
    void updatePlaylistSelection();
    void updatePlaylistStatusMessage();
    void updatePlaylistLastError();
    void updatePlaybackState();
    void updateCurrentPlaybackTrack();
    void updatePlaybackPosition();
    void updatePlaybackDuration();
    void updatePlaybackSeekable();
    void updatePlaybackVolume();
    void updatePlaybackMuted();
    void updatePlaybackError();
    void updatePlaybackStatusMessage();
    void syncPlaybackSelection();
    void setLabelVisibleText(QLabel &label, const QString &text);
    [[nodiscard]] QString playbackStateText() const;
    [[nodiscard]] static int sliderValueFromMs(qint64 valueMs);

    ViewModel::LibraryViewModelProtocol &m_viewModel;
    ViewModel::PlaylistCollectionViewModelProtocol &m_playlistViewModel;
    ViewModel::PlaybackViewModelProtocol &m_playbackViewModel;
    QLineEdit *m_storagePathInput = nullptr;
    QLineEdit *m_scanDirectoryPathInput = nullptr;
    QLineEdit *m_playlistNameInput = nullptr;
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_loadButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_createPlaylistButton = nullptr;
    QPushButton *m_deletePlaylistButton = nullptr;
    QPushButton *m_addSongsToPlaylistButton = nullptr;
    QPushButton *m_playSelectedSongButton = nullptr;
    QPushButton *m_playVisiblePlaylistButton = nullptr;
    QPushButton *m_playButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_previousButton = nullptr;
    QPushButton *m_nextButton = nullptr;
    QPushButton *m_muteButton = nullptr;
    QSlider *m_progressSlider = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_countLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QLabel *m_warningsLabel = nullptr;
    QLabel *m_playlistStatusLabel = nullptr;
    QLabel *m_playlistErrorLabel = nullptr;
    QLabel *m_playbackStateLabel = nullptr;
    QLabel *m_currentPlaybackLabel = nullptr;
    QLabel *m_playbackPositionLabel = nullptr;
    QLabel *m_playbackErrorLabel = nullptr;
    QLabel *m_playbackStatusLabel = nullptr;
    QListView *m_playlistListView = nullptr;
    QListView *m_songListView = nullptr;
    bool m_userSeeking = false;
    bool m_syncingVolume = false;
};

} // namespace AudioPlayer::View
