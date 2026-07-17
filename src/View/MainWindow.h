#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <array>

class QAbstractItemModel;
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

namespace AudioPlayer::View {

struct LibraryCommands
{
    Common::ViewCommand *scan = nullptr;
    Common::ViewCommand *refresh = nullptr;
};

struct PlaylistCommands
{
    Common::ViewCommand *load = nullptr;
    Common::ViewCommand *save = nullptr;
    Common::ViewCommand *create = nullptr;
    Common::ViewCommand *deleteSelected = nullptr;
    Common::ViewCommand *switchSelected = nullptr;
    Common::ViewCommand *addSelectedSongs = nullptr;
    Common::ViewCommand *playSelectedSong = nullptr;
    Common::ViewCommand *playVisiblePlaylist = nullptr;
};

struct PlaybackCommands
{
    Common::ViewCommand *play = nullptr;
    Common::ViewCommand *pause = nullptr;
    Common::ViewCommand *stop = nullptr;
    Common::ViewCommand *previous = nullptr;
    Common::ViewCommand *next = nullptr;
    Common::ViewCommand *toggleMute = nullptr;
    Common::ViewCommand *pauseResume = nullptr;
};

struct AudioEffectsCommands
{
    Common::ViewCommand *resetPlaybackRate = nullptr;
    Common::ViewCommand *resetEqualizer = nullptr;
};

struct ProcessingCommands
{
    Common::ViewCommand *enqueue = nullptr;
    Common::ViewCommand *cancelSelected = nullptr;
    Common::ViewCommand *cancelAll = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void setLibraryCommands(const LibraryCommands &commands);
    void setPlaylistCommands(const PlaylistCommands &commands);
    void setPlaybackCommands(const PlaybackCommands &commands);
    void setAudioEffectsCommands(const AudioEffectsCommands &commands);
    void setProcessingCommands(const ProcessingCommands &commands);

    void setSongModel(QAbstractItemModel *model);
    void setPlaylistModel(QAbstractItemModel *model);
    void setProcessingTaskModel(QAbstractItemModel *model);
    void setPlaylistIdRole(int role) noexcept;

    void setStoragePath(const QString &path);
    void setScanDirectoryPath(const QString &path);
    void setLibraryCount(int count);
    void setLibraryStatusMessage(const QString &message);
    void setLibraryLastError(const QString &error);
    void setLibraryWarnings(const QStringList &warnings);

    void setPlaylistName(const QString &name);
    void setVisiblePlaylistId(const QString &playlistId);
    void setPlaylistStatusMessage(const QString &message);
    void setPlaylistLastError(const QString &error);

    void setPlaybackStateLabel(const QString &stateLabel);
    void setPlaybackHasCurrentTrack(bool hasCurrentTrack);
    void setCurrentPlaybackTitle(const QString &title);
    void setPlaybackPosition(qint64 positionMs);
    void setPlaybackDuration(qint64 durationMs);
    void setPlaybackSeekable(bool seekable);
    void setPlaybackVolumePercent(int volumePercent);
    void setPlaybackMuted(bool muted);
    void setPlaybackLastError(const QString &error);
    void setPlaybackStatusMessage(const QString &message);

    void setPlaybackRateLabels(const QStringList &labels);
    void setEqualizerPresetLabels(const QStringList &labels);
    void setAudioEffectsCapabilities(bool supportsPitchPreservingRate, bool supportsEqualizer);
    void setAudioEffectsPlaybackRate(double rate);
    void setAudioEffectsEqualizerEnabled(bool enabled);
    void setAudioEffectsBandGains(const std::array<double, 5> &gains);
    void setAudioEffectsPreset(const QString &preset);
    void setAudioEffectsStatusMessage(const QString &message);
    void setAudioEffectsLastError(const QString &error);

    void setProcessingInputFilePaths(const QStringList &paths);
    void setProcessingOutputDirectory(const QString &directory);
    void setProcessingOutputFormat(const QString &format);
    void setProcessingHasPendingOrRunningTasks(bool active);
    void setProcessingStatusMessage(const QString &message);
    void setProcessingLastError(const QString &error);

signals:
    void storagePathEdited(QString path);
    void scanPathEdited(QString path);
    void playlistNameEdited(QString name);
    void playlistSelected(QString playlistId);
    void songSelected(int row);
    void seekRequested(qint64 positionMs);
    void volumeChangedByUser(int volumePercent);
    void playbackRateSelected(double rate);
    void equalizerEnabledChangedByUser(bool enabled);
    void equalizerBandGainChanged(int bandIndex, double gainDb);
    void equalizerPresetSelected(QString preset);
    void processingInputsSelected(QStringList paths);
    void processingOutputDirectorySelected(QString directory);
    void processingFormatSelected(QString format);
    void processingTaskSelected(int row);
    void closeWithPendingProcessingRequested();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void bindUiSignals();
    void bindButton(QPushButton &button, Common::ViewCommand *command);
    void setLabelVisibleText(QLabel &label, const QString &text);
    void syncPlaylistSelection();
    void updatePlaybackPositionLabel();
    [[nodiscard]] static int sliderValueFromMs(qint64 valueMs);

    LibraryCommands m_libraryCommands;
    PlaylistCommands m_playlistCommands;
    PlaybackCommands m_playbackCommands;
    AudioEffectsCommands m_audioEffectsCommands;
    ProcessingCommands m_processingCommands;

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
    int m_playlistIdRole = 0;
    QString m_visiblePlaylistId;
    qint64 m_playbackPositionMs = 0;
    qint64 m_playbackDurationMs = 0;
    bool m_playbackSeekable = false;
    bool m_playbackHasCurrentTrack = false;
    bool m_processingHasPendingOrRunningTasks = false;
    bool m_userSeeking = false;
    bool m_syncingStoragePath = false;
    bool m_syncingScanPath = false;
    bool m_syncingPlaylistName = false;
    bool m_syncingVolume = false;
    bool m_syncingAudioEffects = false;
    bool m_syncingProcessingFormat = false;
};

} // namespace AudioPlayer::View
