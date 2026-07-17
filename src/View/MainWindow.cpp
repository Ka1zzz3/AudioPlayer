#include "View/MainWindow.h"

#include "Common/ViewCommand.h"

#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFormLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AudioPlayer::View {

namespace {
constexpr auto defaultStoragePath = "storage/library.json";

QString formatTime(qint64 milliseconds)
{
    const qint64 totalSeconds = std::max(milliseconds, qint64{0}) / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

double playbackRateFromLabel(QString label)
{
    label = label.trimmed();
    if (label.endsWith(QLatin1Char('x'), Qt::CaseInsensitive)) {
        label.chop(1);
    }

    bool ok = false;
    const double rate = label.toDouble(&ok);
    return ok ? rate : 1.0;
}

int sliderValueFromGain(double gainDb)
{
    return static_cast<int>(std::lround(std::clamp(gainDb, -12.0, 12.0) * 10.0));
}

double gainFromSliderValue(int value)
{
    return std::clamp(static_cast<double>(value) / 10.0, -12.0, 12.0);
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    bindUiSignals();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_processingHasPendingOrRunningTasks) {
        QMainWindow::closeEvent(event);
        return;
    }

    const auto answer = QMessageBox::question(this,
                                              tr("Processing tasks are active"),
                                              tr("Pending or running transcoding tasks will be canceled if you exit. Exit now?"),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No);
    if (answer == QMessageBox::Yes) {
        emit closeWithPendingProcessingRequested();
        if (m_processingCommands.cancelAll != nullptr) {
            m_processingCommands.cancelAll->execute();
        }
        QMainWindow::closeEvent(event);
        return;
    }

    event->ignore();
}

void MainWindow::buildUi()
{
    setWindowTitle(tr("AudioPlayer"));
    resize(900, 700);

    auto *centralWidget = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    auto *titleLabel = new QLabel(tr("Audio Library"), centralWidget);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(titleFont.pointSize() + 8);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);

    auto *inputLayout = new QHBoxLayout();
    auto *storageLayout = new QFormLayout();
    auto *scanDirectoryLayout = new QFormLayout();

    auto *storagePathPickerLayout = new QHBoxLayout();
    m_storagePathInput = new QLineEdit(QString::fromLatin1(defaultStoragePath), centralWidget);
    m_selectStorageDirectoryButton = new QPushButton(tr("Select folder"), centralWidget);
    storagePathPickerLayout->addWidget(m_storagePathInput, 1);
    storagePathPickerLayout->addWidget(m_selectStorageDirectoryButton);

    auto *scanDirectoryPickerLayout = new QHBoxLayout();
    m_scanDirectoryPathInput = new QLineEdit(centralWidget);
    m_selectScanDirectoryButton = new QPushButton(tr("Select folder"), centralWidget);
    scanDirectoryPickerLayout->addWidget(m_scanDirectoryPathInput, 1);
    scanDirectoryPickerLayout->addWidget(m_selectScanDirectoryButton);

    storageLayout->addRow(tr("Storage path"), storagePathPickerLayout);
    scanDirectoryLayout->addRow(tr("Scan directory"), scanDirectoryPickerLayout);

    inputLayout->addLayout(storageLayout, 1);
    inputLayout->addLayout(scanDirectoryLayout, 1);

    m_scanButton = new QPushButton(tr("Scan"), centralWidget);
    m_saveButton = new QPushButton(tr("Save"), centralWidget);
    m_loadButton = new QPushButton(tr("Load"), centralWidget);
    m_refreshButton = new QPushButton(tr("Refresh"), centralWidget);
    inputLayout->addWidget(m_scanButton);
    inputLayout->addWidget(m_saveButton);
    inputLayout->addWidget(m_loadButton);
    inputLayout->addWidget(m_refreshButton);
    mainLayout->addLayout(inputLayout);

    m_countLabel = new QLabel(centralWidget);
    m_statusLabel = new QLabel(centralWidget);
    m_errorLabel = new QLabel(centralWidget);
    m_warningsLabel = new QLabel(centralWidget);
    m_errorLabel->setStyleSheet(QStringLiteral("color: #a00000;"));
    m_warningsLabel->setStyleSheet(QStringLiteral("color: #9a6600;"));
    m_warningsLabel->setWordWrap(true);
    mainLayout->addWidget(m_countLabel);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_errorLabel);
    mainLayout->addWidget(m_warningsLabel);

    auto *playlistTitleLabel = new QLabel(tr("Playlists"), centralWidget);
    QFont playlistTitleFont = playlistTitleLabel->font();
    playlistTitleFont.setBold(true);
    playlistTitleLabel->setFont(playlistTitleFont);
    mainLayout->addWidget(playlistTitleLabel);

    auto *playlistLayout = new QHBoxLayout();
    m_playlistListView = new QListView(centralWidget);
    m_playlistListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    playlistLayout->addWidget(m_playlistListView, 1);

    auto *playlistControlsLayout = new QVBoxLayout();
    m_playlistNameInput = new QLineEdit(centralWidget);
    m_playlistNameInput->setPlaceholderText(tr("New playlist name"));
    m_createPlaylistButton = new QPushButton(tr("Create playlist"), centralWidget);
    m_deletePlaylistButton = new QPushButton(tr("Delete playlist"), centralWidget);
    m_addSongsToPlaylistButton = new QPushButton(tr("Add visible songs"), centralWidget);
    m_playSelectedSongButton = new QPushButton(tr("Play selected song"), centralWidget);
    m_playVisiblePlaylistButton = new QPushButton(tr("Play visible playlist"), centralWidget);
    playlistControlsLayout->addWidget(m_playlistNameInput);
    playlistControlsLayout->addWidget(m_createPlaylistButton);
    playlistControlsLayout->addWidget(m_deletePlaylistButton);
    playlistControlsLayout->addWidget(m_addSongsToPlaylistButton);
    playlistControlsLayout->addWidget(m_playVisiblePlaylistButton);
    playlistControlsLayout->addStretch(1);
    playlistLayout->addLayout(playlistControlsLayout);
    mainLayout->addLayout(playlistLayout);

    m_playlistStatusLabel = new QLabel(centralWidget);
    m_playlistErrorLabel = new QLabel(centralWidget);
    m_playlistErrorLabel->setStyleSheet(QStringLiteral("color: #a00000;"));
    mainLayout->addWidget(m_playlistStatusLabel);
    mainLayout->addWidget(m_playlistErrorLabel);

    m_songListView = new QListView(centralWidget);
    m_songListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_songListView, 1);

    auto *playbackTitleLabel = new QLabel(tr("Playback"), centralWidget);
    QFont playbackTitleFont = playbackTitleLabel->font();
    playbackTitleFont.setBold(true);
    playbackTitleLabel->setFont(playbackTitleFont);
    mainLayout->addWidget(playbackTitleLabel);

    auto *playbackButtonsLayout = new QHBoxLayout();
    m_previousButton = new QPushButton(tr("Previous"), centralWidget);
    m_pauseButton = new QPushButton(tr("Pause/Resume"), centralWidget);
    m_stopButton = new QPushButton(tr("Stop"), centralWidget);
    m_nextButton = new QPushButton(tr("Next"), centralWidget);
    m_muteButton = new QPushButton(tr("Mute"), centralWidget);
    playbackButtonsLayout->addWidget(m_previousButton);
    playbackButtonsLayout->addWidget(m_playSelectedSongButton);
    playbackButtonsLayout->addWidget(m_pauseButton);
    playbackButtonsLayout->addWidget(m_stopButton);
    playbackButtonsLayout->addWidget(m_nextButton);
    playbackButtonsLayout->addWidget(m_muteButton);
    mainLayout->addLayout(playbackButtonsLayout);

    auto *progressLayout = new QHBoxLayout();
    m_progressSlider = new QSlider(Qt::Horizontal, centralWidget);
    m_progressSlider->setRange(0, 0);
    m_progressSlider->setEnabled(false);
    m_playbackPositionLabel = new QLabel(centralWidget);
    progressLayout->addWidget(m_progressSlider, 1);
    progressLayout->addWidget(m_playbackPositionLabel);
    mainLayout->addLayout(progressLayout);

    auto *volumeLayout = new QHBoxLayout();
    auto *volumeLabel = new QLabel(tr("Volume"), centralWidget);
    m_volumeSlider = new QSlider(Qt::Horizontal, centralWidget);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    volumeLayout->addWidget(volumeLabel);
    volumeLayout->addWidget(m_volumeSlider, 1);
    mainLayout->addLayout(volumeLayout);

    m_playbackStateLabel = new QLabel(centralWidget);
    m_currentPlaybackLabel = new QLabel(centralWidget);
    m_playbackStatusLabel = new QLabel(centralWidget);
    m_playbackErrorLabel = new QLabel(centralWidget);
    m_playbackErrorLabel->setStyleSheet(QStringLiteral("color: #a00000;"));
    mainLayout->addWidget(m_playbackStateLabel);
    mainLayout->addWidget(m_currentPlaybackLabel);
    mainLayout->addWidget(m_playbackStatusLabel);
    mainLayout->addWidget(m_playbackErrorLabel);

    auto *effectsTitleLabel = new QLabel(tr("Playback speed and EQ"), centralWidget);
    QFont effectsTitleFont = effectsTitleLabel->font();
    effectsTitleFont.setBold(true);
    effectsTitleLabel->setFont(effectsTitleFont);
    mainLayout->addWidget(effectsTitleLabel);

    auto *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel(tr("Speed"), centralWidget));
    m_playbackRateComboBox = new QComboBox(centralWidget);
    m_resetPlaybackRateButton = new QPushButton(tr("Reset 1x"), centralWidget);
    speedLayout->addWidget(m_playbackRateComboBox);
    speedLayout->addWidget(m_resetPlaybackRateButton);
    speedLayout->addStretch(1);
    mainLayout->addLayout(speedLayout);

    auto *equalizerControlsLayout = new QHBoxLayout();
    m_equalizerEnabledCheckBox = new QCheckBox(tr("Enable EQ"), centralWidget);
    m_equalizerPresetComboBox = new QComboBox(centralWidget);
    m_resetEqualizerButton = new QPushButton(tr("Reset Flat"), centralWidget);
    equalizerControlsLayout->addWidget(m_equalizerEnabledCheckBox);
    equalizerControlsLayout->addWidget(new QLabel(tr("Preset"), centralWidget));
    equalizerControlsLayout->addWidget(m_equalizerPresetComboBox);
    equalizerControlsLayout->addWidget(m_resetEqualizerButton);
    equalizerControlsLayout->addStretch(1);
    mainLayout->addLayout(equalizerControlsLayout);

    auto *equalizerSlidersLayout = new QHBoxLayout();
    const QStringList equalizerBandLabels = {tr("60 Hz"), tr("230 Hz"), tr("910 Hz"), tr("3.6 kHz"), tr("14 kHz")};
    for (int index = 0; index < static_cast<int>(m_equalizerSliders.size()); ++index) {
        auto *bandLayout = new QVBoxLayout();
        auto *bandLabel = new QLabel(equalizerBandLabels.at(index), centralWidget);
        m_equalizerSliders[static_cast<std::size_t>(index)] = new QSlider(Qt::Vertical, centralWidget);
        m_equalizerSliders[static_cast<std::size_t>(index)]->setRange(-120, 120);
        m_equalizerSliders[static_cast<std::size_t>(index)]->setValue(0);
        m_equalizerSliders[static_cast<std::size_t>(index)]->setTickInterval(60);
        m_equalizerSliders[static_cast<std::size_t>(index)]->setTickPosition(QSlider::TicksBothSides);
        bandLayout->addWidget(bandLabel);
        bandLayout->addWidget(m_equalizerSliders[static_cast<std::size_t>(index)]);
        equalizerSlidersLayout->addLayout(bandLayout);
    }
    equalizerSlidersLayout->addStretch(1);
    mainLayout->addLayout(equalizerSlidersLayout);

    m_audioEffectsStatusLabel = new QLabel(centralWidget);
    m_audioEffectsErrorLabel = new QLabel(centralWidget);
    m_audioEffectsErrorLabel->setStyleSheet(QStringLiteral("color: #a00000;"));
    mainLayout->addWidget(m_audioEffectsStatusLabel);
    mainLayout->addWidget(m_audioEffectsErrorLabel);

    auto *processingTitleLabel = new QLabel(tr("Transcoding"), centralWidget);
    QFont processingTitleFont = processingTitleLabel->font();
    processingTitleFont.setBold(true);
    processingTitleLabel->setFont(processingTitleFont);
    mainLayout->addWidget(processingTitleLabel);

    auto *processingControlsLayout = new QHBoxLayout();
    m_selectProcessingInputsButton = new QPushButton(tr("Select input files"), centralWidget);
    m_selectProcessingOutputDirectoryButton = new QPushButton(tr("Select output directory"), centralWidget);
    m_processingFormatComboBox = new QComboBox(centralWidget);
    m_processingFormatComboBox->addItems({QStringLiteral("mp3"), QStringLiteral("wav"), QStringLiteral("flac")});
    m_enqueueProcessingButton = new QPushButton(tr("Enqueue transcode"), centralWidget);
    m_cancelSelectedProcessingButton = new QPushButton(tr("Cancel selected"), centralWidget);
    m_cancelAllProcessingButton = new QPushButton(tr("Cancel all"), centralWidget);
    processingControlsLayout->addWidget(m_selectProcessingInputsButton);
    processingControlsLayout->addWidget(m_selectProcessingOutputDirectoryButton);
    processingControlsLayout->addWidget(m_processingFormatComboBox);
    processingControlsLayout->addWidget(m_enqueueProcessingButton);
    processingControlsLayout->addWidget(m_cancelSelectedProcessingButton);
    processingControlsLayout->addWidget(m_cancelAllProcessingButton);
    mainLayout->addLayout(processingControlsLayout);

    m_processingInputSummaryLabel = new QLabel(centralWidget);
    m_processingOutputDirectoryLabel = new QLabel(centralWidget);
    m_processingStatusLabel = new QLabel(centralWidget);
    m_processingErrorLabel = new QLabel(centralWidget);
    m_processingErrorLabel->setStyleSheet(QStringLiteral("color: #a00000;"));
    mainLayout->addWidget(m_processingInputSummaryLabel);
    mainLayout->addWidget(m_processingOutputDirectoryLabel);
    mainLayout->addWidget(m_processingStatusLabel);
    mainLayout->addWidget(m_processingErrorLabel);

    m_processingTaskListView = new QListView(centralWidget);
    m_processingTaskListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mainLayout->addWidget(m_processingTaskListView, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::bindUiSignals()
{
    connect(m_storagePathInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!m_syncingStoragePath) {
            emit storagePathEdited(text);
        }
    });
    connect(m_scanDirectoryPathInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!m_syncingScanPath) {
            emit scanPathEdited(text);
        }
    });
    connect(m_playlistNameInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        if (!m_syncingPlaylistName) {
            emit playlistNameEdited(text);
        }
    });

    connect(m_selectScanDirectoryButton, &QPushButton::clicked, this, [this]() {
        const QString directory = QFileDialog::getExistingDirectory(this,
                                                                    tr("Select scan directory"),
                                                                    m_scanDirectoryPathInput->text());
        if (!directory.isEmpty()) {
            m_scanDirectoryPathInput->setText(directory);
        }
    });
    connect(m_selectStorageDirectoryButton, &QPushButton::clicked, this, [this]() {
        const QFileInfo currentStoragePath(m_storagePathInput->text());
        const QString initialDirectory = currentStoragePath.absoluteDir().exists()
                                             ? currentStoragePath.absoluteDir().absolutePath()
                                             : QString();
        const QString directory = QFileDialog::getExistingDirectory(this,
                                                                    tr("Select storage folder"),
                                                                    initialDirectory);
        if (!directory.isEmpty()) {
            m_storagePathInput->setText(QDir(directory).filePath(QStringLiteral("library.json")));
        }
    });

    connect(m_progressSlider, &QSlider::sliderPressed, this, [this]() { m_userSeeking = true; });
    connect(m_progressSlider, &QSlider::sliderReleased, this, [this]() {
        m_userSeeking = false;
        emit seekRequested(m_progressSlider->value());
        updatePlaybackPositionLabel();
    });
    connect(m_progressSlider, &QSlider::sliderMoved, this, [this](int value) {
        m_playbackPositionLabel->setText(tr("%1 / %2").arg(formatTime(value), formatTime(m_playbackDurationMs)));
    });
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!m_syncingVolume) {
            emit volumeChangedByUser(value);
        }
    });

    connect(m_playbackRateComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (!m_syncingAudioEffects && index >= 0) {
            emit playbackRateSelected(m_playbackRateComboBox->itemData(index).toDouble());
        }
    });
    connect(m_equalizerEnabledCheckBox, &QCheckBox::toggled, this, [this](bool enabled) {
        if (!m_syncingAudioEffects) {
            emit equalizerEnabledChangedByUser(enabled);
        }
    });
    for (int index = 0; index < static_cast<int>(m_equalizerSliders.size()); ++index) {
        QSlider *slider = m_equalizerSliders[static_cast<std::size_t>(index)];
        connect(slider, &QSlider::valueChanged, this, [this, index](int value) {
            if (!m_syncingAudioEffects) {
                emit equalizerBandGainChanged(index, gainFromSliderValue(value));
            }
        });
    }
    connect(m_equalizerPresetComboBox, &QComboBox::currentTextChanged, this, [this](const QString &preset) {
        if (!m_syncingAudioEffects) {
            emit equalizerPresetSelected(preset);
        }
    });

    connect(m_selectProcessingInputsButton, &QPushButton::clicked, this, [this]() {
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Select audio files to transcode"));
        if (!files.isEmpty()) {
            emit processingInputsSelected(files);
        }
    });
    connect(m_selectProcessingOutputDirectoryButton, &QPushButton::clicked, this, [this]() {
        const QString directory = QFileDialog::getExistingDirectory(this, tr("Select transcode output directory"));
        if (!directory.isEmpty()) {
            emit processingOutputDirectorySelected(directory);
        }
    });
    connect(m_processingFormatComboBox, &QComboBox::currentTextChanged, this, [this](const QString &format) {
        if (!m_syncingProcessingFormat) {
            emit processingFormatSelected(format);
        }
    });
}

void MainWindow::setLibraryCommands(const LibraryCommands &commands)
{
    m_libraryCommands = commands;
    bindButton(*m_scanButton, m_libraryCommands.scan);
    bindButton(*m_refreshButton, m_libraryCommands.refresh);
}

void MainWindow::setPlaylistCommands(const PlaylistCommands &commands)
{
    m_playlistCommands = commands;
    bindButton(*m_loadButton, m_playlistCommands.load);
    bindButton(*m_saveButton, m_playlistCommands.save);
    bindButton(*m_createPlaylistButton, m_playlistCommands.create);
    bindButton(*m_deletePlaylistButton, m_playlistCommands.deleteSelected);
    bindButton(*m_addSongsToPlaylistButton, m_playlistCommands.addSelectedSongs);
    bindButton(*m_playSelectedSongButton, m_playlistCommands.playSelectedSong);
    bindButton(*m_playVisiblePlaylistButton, m_playlistCommands.playVisiblePlaylist);
}

void MainWindow::setPlaybackCommands(const PlaybackCommands &commands)
{
    m_playbackCommands = commands;
    bindButton(*m_pauseButton, m_playbackCommands.pauseResume);
    bindButton(*m_stopButton, m_playbackCommands.stop);
    bindButton(*m_previousButton, m_playbackCommands.previous);
    bindButton(*m_nextButton, m_playbackCommands.next);
    bindButton(*m_muteButton, m_playbackCommands.toggleMute);
}

void MainWindow::setAudioEffectsCommands(const AudioEffectsCommands &commands)
{
    m_audioEffectsCommands = commands;
    bindButton(*m_resetPlaybackRateButton, m_audioEffectsCommands.resetPlaybackRate);
    bindButton(*m_resetEqualizerButton, m_audioEffectsCommands.resetEqualizer);
}

void MainWindow::setProcessingCommands(const ProcessingCommands &commands)
{
    m_processingCommands = commands;
    bindButton(*m_enqueueProcessingButton, m_processingCommands.enqueue);
    bindButton(*m_cancelSelectedProcessingButton, m_processingCommands.cancelSelected);
    bindButton(*m_cancelAllProcessingButton, m_processingCommands.cancelAll);
    setProcessingHasPendingOrRunningTasks(m_processingHasPendingOrRunningTasks);
}

void MainWindow::setSongModel(QAbstractItemModel *model)
{
    m_songListView->setModel(model);
    connect(m_songListView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current) {
        emit songSelected(current.isValid() ? current.row() : -1);
    });
}

void MainWindow::setPlaylistModel(QAbstractItemModel *model)
{
    m_playlistListView->setModel(model);
    connect(m_playlistListView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current) {
        if (!current.isValid()) {
            return;
        }
        const QString playlistId = current.data(m_playlistIdRole).toString();
        if (!playlistId.isEmpty()) {
            emit playlistSelected(playlistId);
        }
    });
    syncPlaylistSelection();
}

void MainWindow::setProcessingTaskModel(QAbstractItemModel *model)
{
    m_processingTaskListView->setModel(model);
    connect(m_processingTaskListView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current) {
        emit processingTaskSelected(current.isValid() ? current.row() : -1);
    });
}

void MainWindow::setPlaylistIdRole(int role) noexcept
{
    m_playlistIdRole = role;
}

void MainWindow::setStoragePath(const QString &path)
{
    const QSignalBlocker blocker(m_storagePathInput);
    Q_UNUSED(blocker)
    m_syncingStoragePath = true;
    m_storagePathInput->setText(path);
    m_syncingStoragePath = false;
}

void MainWindow::setScanDirectoryPath(const QString &path)
{
    const QSignalBlocker blocker(m_scanDirectoryPathInput);
    Q_UNUSED(blocker)
    m_syncingScanPath = true;
    m_scanDirectoryPathInput->setText(path);
    m_syncingScanPath = false;
}

void MainWindow::setLibraryCount(int count)
{
    m_countLabel->setText(tr("%n song(s)", nullptr, count));
}

void MainWindow::setLibraryStatusMessage(const QString &message)
{
    setLabelVisibleText(*m_statusLabel, message);
}

void MainWindow::setLibraryLastError(const QString &error)
{
    setLabelVisibleText(*m_errorLabel, error);
}

void MainWindow::setLibraryWarnings(const QStringList &warnings)
{
    QStringList lines;
    lines.reserve(warnings.size());
    for (const QString &warning : warnings) {
        lines.append(tr("Warning: %1").arg(warning));
    }
    setLabelVisibleText(*m_warningsLabel, lines.join(QLatin1Char('\n')));
}

void MainWindow::setPlaylistName(const QString &name)
{
    const QSignalBlocker blocker(m_playlistNameInput);
    Q_UNUSED(blocker)
    m_syncingPlaylistName = true;
    m_playlistNameInput->setText(name);
    m_syncingPlaylistName = false;
}

void MainWindow::setVisiblePlaylistId(const QString &playlistId)
{
    m_visiblePlaylistId = playlistId;
    syncPlaylistSelection();
}

void MainWindow::setPlaylistStatusMessage(const QString &message)
{
    setLabelVisibleText(*m_playlistStatusLabel, message.isEmpty() ? QString() : tr("Playlist: %1").arg(message));
}

void MainWindow::setPlaylistLastError(const QString &error)
{
    setLabelVisibleText(*m_playlistErrorLabel, error.isEmpty() ? QString() : tr("Playlist error: %1").arg(error));
}

void MainWindow::setPlaybackStateLabel(const QString &stateLabel)
{
    m_playbackStateLabel->setText(tr("Playback state: %1").arg(stateLabel));
}

void MainWindow::setPlaybackHasCurrentTrack(bool hasCurrentTrack)
{
    m_playbackHasCurrentTrack = hasCurrentTrack;
    m_pauseButton->setEnabled(hasCurrentTrack && m_playbackCommands.pauseResume != nullptr);
}

void MainWindow::setCurrentPlaybackTitle(const QString &title)
{
    setLabelVisibleText(*m_currentPlaybackLabel, title.isEmpty() ? QString() : tr("Now playing: %1").arg(title));
}

void MainWindow::setPlaybackPosition(qint64 positionMs)
{
    m_playbackPositionMs = positionMs;
    const int value = sliderValueFromMs(positionMs);
    if (!m_userSeeking && m_progressSlider->value() != value) {
        m_progressSlider->setValue(value);
    }
    updatePlaybackPositionLabel();
}

void MainWindow::setPlaybackDuration(qint64 durationMs)
{
    m_playbackDurationMs = durationMs;
    m_progressSlider->setRange(0, sliderValueFromMs(durationMs));
    updatePlaybackPositionLabel();
}

void MainWindow::setPlaybackSeekable(bool seekable)
{
    m_playbackSeekable = seekable;
    m_progressSlider->setEnabled(m_playbackSeekable && m_playbackDurationMs > 0);
}

void MainWindow::setPlaybackVolumePercent(int volumePercent)
{
    const QSignalBlocker blocker(m_volumeSlider);
    Q_UNUSED(blocker)
    m_syncingVolume = true;
    m_volumeSlider->setValue(volumePercent);
    m_syncingVolume = false;
}

void MainWindow::setPlaybackMuted(bool muted)
{
    m_muteButton->setText(muted ? tr("Unmute") : tr("Mute"));
}

void MainWindow::setPlaybackLastError(const QString &error)
{
    setLabelVisibleText(*m_playbackErrorLabel, error);
}

void MainWindow::setPlaybackStatusMessage(const QString &message)
{
    setLabelVisibleText(*m_playbackStatusLabel, message);
}

void MainWindow::setPlaybackRateLabels(const QStringList &labels)
{
    const QSignalBlocker blocker(m_playbackRateComboBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    m_playbackRateComboBox->clear();
    for (const QString &label : labels) {
        m_playbackRateComboBox->addItem(label, playbackRateFromLabel(label));
    }
    m_syncingAudioEffects = false;
}

void MainWindow::setEqualizerPresetLabels(const QStringList &labels)
{
    const QSignalBlocker blocker(m_equalizerPresetComboBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    m_equalizerPresetComboBox->clear();
    m_equalizerPresetComboBox->addItems(labels);
    m_syncingAudioEffects = false;
}

void MainWindow::setAudioEffectsCapabilities(bool supportsPitchPreservingRate, bool supportsEqualizer)
{
    m_playbackRateComboBox->setEnabled(supportsPitchPreservingRate);
    m_resetPlaybackRateButton->setEnabled(supportsPitchPreservingRate && m_audioEffectsCommands.resetPlaybackRate != nullptr);
    m_equalizerEnabledCheckBox->setEnabled(supportsEqualizer);
    m_equalizerPresetComboBox->setEnabled(supportsEqualizer);
    m_resetEqualizerButton->setEnabled(supportsEqualizer && m_audioEffectsCommands.resetEqualizer != nullptr);
    for (QSlider *slider : m_equalizerSliders) {
        if (slider != nullptr) {
            slider->setEnabled(supportsEqualizer);
        }
    }
}

void MainWindow::setAudioEffectsPlaybackRate(double rate)
{
    const QSignalBlocker blocker(m_playbackRateComboBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    for (int index = 0; index < m_playbackRateComboBox->count(); ++index) {
        if (std::abs(m_playbackRateComboBox->itemData(index).toDouble() - rate) < 0.001) {
            m_playbackRateComboBox->setCurrentIndex(index);
            break;
        }
    }
    m_syncingAudioEffects = false;
}

void MainWindow::setAudioEffectsEqualizerEnabled(bool enabled)
{
    const QSignalBlocker blocker(m_equalizerEnabledCheckBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    m_equalizerEnabledCheckBox->setChecked(enabled);
    m_syncingAudioEffects = false;
}

void MainWindow::setAudioEffectsBandGains(const std::array<double, 5> &gains)
{
    m_syncingAudioEffects = true;
    for (std::size_t index = 0; index < gains.size(); ++index) {
        QSlider *slider = m_equalizerSliders[index];
        if (slider == nullptr) {
            continue;
        }
        const QSignalBlocker blocker(slider);
        Q_UNUSED(blocker)
        slider->setValue(sliderValueFromGain(gains[index]));
    }
    m_syncingAudioEffects = false;
}

void MainWindow::setAudioEffectsPreset(const QString &preset)
{
    const QSignalBlocker blocker(m_equalizerPresetComboBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    const int index = m_equalizerPresetComboBox->findText(preset);
    if (index >= 0) {
        m_equalizerPresetComboBox->setCurrentIndex(index);
    }
    m_syncingAudioEffects = false;
}

void MainWindow::setAudioEffectsStatusMessage(const QString &message)
{
    setLabelVisibleText(*m_audioEffectsStatusLabel, message.isEmpty() ? QString() : tr("Effects: %1").arg(message));
}

void MainWindow::setAudioEffectsLastError(const QString &error)
{
    setLabelVisibleText(*m_audioEffectsErrorLabel, error.isEmpty() ? QString() : tr("Effects error: %1").arg(error));
}

void MainWindow::setProcessingInputFilePaths(const QStringList &paths)
{
    setLabelVisibleText(*m_processingInputSummaryLabel,
                        paths.isEmpty() ? QString() : tr("Transcode inputs: %n file(s)", nullptr, paths.size()));
}

void MainWindow::setProcessingOutputDirectory(const QString &directory)
{
    setLabelVisibleText(*m_processingOutputDirectoryLabel,
                        directory.isEmpty() ? QString() : tr("Transcode output: %1").arg(directory));
}

void MainWindow::setProcessingOutputFormat(const QString &format)
{
    if (m_processingFormatComboBox->currentText() == format) {
        return;
    }

    const QSignalBlocker blocker(m_processingFormatComboBox);
    Q_UNUSED(blocker)
    m_syncingProcessingFormat = true;
    const int index = m_processingFormatComboBox->findText(format);
    if (index >= 0) {
        m_processingFormatComboBox->setCurrentIndex(index);
    }
    m_syncingProcessingFormat = false;
}

void MainWindow::setProcessingHasPendingOrRunningTasks(bool active)
{
    m_processingHasPendingOrRunningTasks = active;
    m_cancelAllProcessingButton->setEnabled(active && m_processingCommands.cancelAll != nullptr);
}

void MainWindow::setProcessingStatusMessage(const QString &message)
{
    setLabelVisibleText(*m_processingStatusLabel, message.isEmpty() ? QString() : tr("Processing: %1").arg(message));
}

void MainWindow::setProcessingLastError(const QString &error)
{
    setLabelVisibleText(*m_processingErrorLabel, error.isEmpty() ? QString() : tr("Processing error: %1").arg(error));
}

void MainWindow::bindButton(QPushButton &button, Common::ViewCommand *command)
{
    button.disconnect();
    if (command == nullptr) {
        button.setEnabled(false);
        return;
    }

    button.setEnabled(command->isEnabled());
    connect(&button, &QPushButton::clicked, command, &Common::ViewCommand::execute);
    connect(command, &Common::ViewCommand::enabledChanged, &button, [&button, command]() {
        button.setEnabled(command->isEnabled());
    });
}

void MainWindow::setLabelVisibleText(QLabel &label, const QString &text)
{
    label.setText(text);
    label.setVisible(!text.isEmpty());
}

void MainWindow::syncPlaylistSelection()
{
    if (m_playlistListView->model() == nullptr || m_visiblePlaylistId.isEmpty()) {
        return;
    }

    for (int row = 0; row < m_playlistListView->model()->rowCount(); ++row) {
        const QModelIndex index = m_playlistListView->model()->index(row, 0);
        if (index.data(m_playlistIdRole).toString() == m_visiblePlaylistId) {
            if (m_playlistListView->currentIndex() != index) {
                const QSignalBlocker blocker(m_playlistListView->selectionModel());
                Q_UNUSED(blocker)
                m_playlistListView->setCurrentIndex(index);
            }
            return;
        }
    }
}

void MainWindow::updatePlaybackPositionLabel()
{
    const int displayValue = m_userSeeking ? m_progressSlider->value() : sliderValueFromMs(m_playbackPositionMs);
    m_playbackPositionLabel->setText(tr("%1 / %2").arg(formatTime(displayValue), formatTime(m_playbackDurationMs)));
}

int MainWindow::sliderValueFromMs(qint64 valueMs)
{
    const qint64 clamped = std::clamp(valueMs, qint64{0}, static_cast<qint64>(std::numeric_limits<int>::max()));
    return static_cast<int>(clamped);
}

} // namespace AudioPlayer::View
