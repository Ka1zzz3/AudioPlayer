#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <array>

class QLabel;
class QCheckBox;
class QComboBox;
class QCloseEvent;
class QLineEdit;
class QListView;
class QPushButton;
class QSlider;
class QWidget;

namespace AudioPlayer::Common {
class ViewCommand;
}

namespace AudioPlayer::ViewModel {
class AudioEffectsViewModelProtocol;
class LibraryViewModelProtocol;
class PlaybackViewModelProtocol;
class PlaylistCollectionViewModelProtocol;
class ProcessingViewModelProtocol;
enum class PlaybackState;
}

namespace AudioPlayer::View {

class MainWindow : public QMainWindow
{
public:
    MainWindow(ViewModel::LibraryViewModelProtocol &libraryViewModel,
               ViewModel::PlaylistCollectionViewModelProtocol &playlistViewModel,
               ViewModel::PlaybackViewModelProtocol &playbackViewModel,
               ViewModel::AudioEffectsViewModelProtocol &audioEffectsViewModel,
               ViewModel::ProcessingViewModelProtocol &processingViewModel,
               QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void bindViewModel();
    void bindLibraryViewModel();
    void bindPlaylistCollectionViewModel();
    void bindPlaybackViewModel();
    void bindAudioEffectsViewModel();
    void bindProcessingViewModel();
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
    void updateAudioEffectsCapabilities();
    void updateAudioEffectsPlaybackRate();
    void updateAudioEffectsEqualizerEnabled();
    void updateAudioEffectsBandGains();
    void updateAudioEffectsPreset();
    void updateAudioEffectsStatusMessage();
    void updateAudioEffectsLastError();
    void updateProcessingInputSummary();
    void updateProcessingOutputDirectory();
    void updateProcessingOutputFormat();
    void updateProcessingActiveState();
    void updateProcessingStatusMessage();
    void updateProcessingLastError();
    void setLabelVisibleText(QLabel &label, const QString &text);
    [[nodiscard]] QString playbackStateText() const;
    [[nodiscard]] static int sliderValueFromMs(qint64 valueMs);

    ViewModel::LibraryViewModelProtocol &m_viewModel;
    ViewModel::PlaylistCollectionViewModelProtocol &m_playlistViewModel;
    ViewModel::PlaybackViewModelProtocol &m_playbackViewModel;
    ViewModel::AudioEffectsViewModelProtocol &m_audioEffectsViewModel;
    ViewModel::ProcessingViewModelProtocol &m_processingViewModel;
    QLineEdit *m_storagePathInput = nullptr;
    QLineEdit *m_scanDirectoryPathInput = nullptr;
    QLineEdit *m_playlistNameInput = nullptr;
    QPushButton *m_selectStorageDirectoryButton = nullptr;
    QPushButton *m_selectScanDirectoryButton = nullptr;
    QPushButton *m_scanButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_loadButton = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_createPlaylistButton = nullptr;
    QPushButton *m_deletePlaylistButton = nullptr;
    QPushButton *m_addSongsToPlaylistButton = nullptr;
    QPushButton *m_playSelectedSongButton = nullptr;
    QPushButton *m_playVisiblePlaylistButton = nullptr;
    QPushButton *m_pauseButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_previousButton = nullptr;
    QPushButton *m_nextButton = nullptr;
    QPushButton *m_muteButton = nullptr;
    QPushButton *m_resetPlaybackRateButton = nullptr;
    QPushButton *m_resetEqualizerButton = nullptr;
    QPushButton *m_selectProcessingInputsButton = nullptr;
    QPushButton *m_selectProcessingOutputDirectoryButton = nullptr;
    QPushButton *m_enqueueProcessingButton = nullptr;
    QPushButton *m_cancelSelectedProcessingButton = nullptr;
    QPushButton *m_cancelAllProcessingButton = nullptr;
    QSlider *m_progressSlider = nullptr;
    QSlider *m_volumeSlider = nullptr;
    std::array<QSlider *, 5> m_equalizerSliders = {};
    QCheckBox *m_equalizerEnabledCheckBox = nullptr;
    QComboBox *m_playbackRateComboBox = nullptr;
    QComboBox *m_equalizerPresetComboBox = nullptr;
    QComboBox *m_processingFormatComboBox = nullptr;
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
    QLabel *m_audioEffectsStatusLabel = nullptr;
    QLabel *m_audioEffectsErrorLabel = nullptr;
    QLabel *m_processingInputSummaryLabel = nullptr;
    QLabel *m_processingOutputDirectoryLabel = nullptr;
    QLabel *m_processingStatusLabel = nullptr;
    QLabel *m_processingErrorLabel = nullptr;
    QListView *m_playlistListView = nullptr;
    QListView *m_songListView = nullptr;
    QListView *m_processingTaskListView = nullptr;
    bool m_userSeeking = false;
    bool m_syncingVolume = false;
    bool m_syncingAudioEffects = false;
};

} // namespace AudioPlayer::View
