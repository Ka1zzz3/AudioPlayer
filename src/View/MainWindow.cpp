#include "View/MainWindow.h"

#include "Common/ViewCommand.h"
#include "ViewModel/AudioEffectsViewModelProtocol.h"
#include "ViewModel/LibraryViewModelProtocol.h"
#include "ViewModel/PlaybackViewModelProtocol.h"
#include "ViewModel/PlaylistCollectionViewModelProtocol.h"
#include "ViewModel/ProcessingViewModelProtocol.h"
#include "ViewModel/PlaybackState.h"

#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QListView>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QWidget>

#include <algorithm>
#include <array>
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

MainWindow::MainWindow(ViewModel::LibraryViewModelProtocol &libraryViewModel,
                       ViewModel::PlaylistCollectionViewModelProtocol &playlistViewModel,
                       ViewModel::PlaybackViewModelProtocol &playbackViewModel,
                       ViewModel::AudioEffectsViewModelProtocol &audioEffectsViewModel,
                       ViewModel::ProcessingViewModelProtocol &processingViewModel,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_viewModel(libraryViewModel)
    , m_playlistViewModel(playlistViewModel)
    , m_playbackViewModel(playbackViewModel)
    , m_audioEffectsViewModel(audioEffectsViewModel)
    , m_processingViewModel(processingViewModel)
{
    buildUi();
    bindViewModel();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_processingViewModel.hasPendingOrRunningTasks()) {
        QMainWindow::closeEvent(event);
        return;
    }

    const auto answer = QMessageBox::question(this,
                                              tr("Processing tasks are active"),
                                              tr("Pending or running transcoding tasks will be canceled if you exit. Exit now?"),
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No);
    if (answer == QMessageBox::Yes) {
        if (m_processingViewModel.cancelAllCommand() != nullptr) {
            m_processingViewModel.cancelAllCommand()->execute();
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

    m_storagePathInput = new QLineEdit(QString::fromLatin1(defaultStoragePath), centralWidget);
    m_scanDirectoryPathInput = new QLineEdit(centralWidget);
    storageLayout->addRow(tr("Storage path"), m_storagePathInput);
    scanDirectoryLayout->addRow(tr("Scan directory"), m_scanDirectoryPathInput);

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
    m_playlistListView->setModel(m_playlistViewModel.playlists());
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
    playlistControlsLayout->addWidget(m_playSelectedSongButton);
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
    m_songListView->setModel(m_viewModel.songs());
    mainLayout->addWidget(m_songListView, 1);

    auto *playbackTitleLabel = new QLabel(tr("Playback"), centralWidget);
    QFont playbackTitleFont = playbackTitleLabel->font();
    playbackTitleFont.setBold(true);
    playbackTitleLabel->setFont(playbackTitleFont);
    mainLayout->addWidget(playbackTitleLabel);

    auto *playbackButtonsLayout = new QHBoxLayout();
    m_previousButton = new QPushButton(tr("Previous"), centralWidget);
    m_playButton = new QPushButton(tr("Play"), centralWidget);
    m_pauseButton = new QPushButton(tr("Pause"), centralWidget);
    m_stopButton = new QPushButton(tr("Stop"), centralWidget);
    m_nextButton = new QPushButton(tr("Next"), centralWidget);
    m_muteButton = new QPushButton(tr("Mute"), centralWidget);
    playbackButtonsLayout->addWidget(m_previousButton);
    playbackButtonsLayout->addWidget(m_playButton);
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
    m_processingTaskListView->setModel(m_processingViewModel.tasks());
    mainLayout->addWidget(m_processingTaskListView, 1);

    setCentralWidget(centralWidget);
}

void MainWindow::bindViewModel()
{
    bindLibraryViewModel();
    bindPlaylistCollectionViewModel();
    bindPlaybackViewModel();
    bindAudioEffectsViewModel();
    bindProcessingViewModel();
}

void MainWindow::bindLibraryViewModel()
{
    m_viewModel.setScanDirectoryPath(m_scanDirectoryPathInput->text());

    bindLineEdit(*m_scanDirectoryPathInput,
                 &ViewModel::LibraryViewModelProtocol::scanDirectoryPath,
                 &ViewModel::LibraryViewModelProtocol::setScanDirectoryPath,
                 &ViewModel::LibraryViewModelProtocol::scanDirectoryPathChanged);

    bindButton(*m_scanButton, m_viewModel.scanCommand());
    bindButton(*m_refreshButton, m_viewModel.refreshCommand());

    connect(&m_viewModel, &ViewModel::LibraryViewModelProtocol::countChanged, this, &MainWindow::updateCount);
    connect(&m_viewModel,
            &ViewModel::LibraryViewModelProtocol::statusMessageChanged,
            this,
            &MainWindow::updateStatusMessage);
    connect(&m_viewModel, &ViewModel::LibraryViewModelProtocol::lastErrorChanged, this, &MainWindow::updateLastError);
    connect(&m_viewModel, &ViewModel::LibraryViewModelProtocol::warningsChanged, this, &MainWindow::updateWarnings);

    updateCount();
    updateStatusMessage();
    updateLastError();
    updateWarnings();
}

void MainWindow::bindPlaylistCollectionViewModel()
{
    m_playlistViewModel.setStoragePath(m_storagePathInput->text());

    connect(m_storagePathInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_playlistViewModel.setStoragePath(text);
    });
    connect(&m_playlistViewModel,
            &ViewModel::PlaylistCollectionViewModelProtocol::storagePathChanged,
            this,
            [this]() {
                if (m_storagePathInput->text() != m_playlistViewModel.storagePath()) {
                    m_storagePathInput->setText(m_playlistViewModel.storagePath());
                }
            });

    connect(m_playlistNameInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_playlistViewModel.setNewPlaylistName(text);
    });
    connect(&m_playlistViewModel,
            &ViewModel::PlaylistCollectionViewModelProtocol::newPlaylistNameChanged,
            this,
            [this]() {
                if (m_playlistNameInput->text() != m_playlistViewModel.newPlaylistName()) {
                    m_playlistNameInput->setText(m_playlistViewModel.newPlaylistName());
                }
            });

    bindButton(*m_loadButton, m_playlistViewModel.loadCommand());
    bindButton(*m_saveButton, m_playlistViewModel.saveCommand());
    bindButton(*m_createPlaylistButton, m_playlistViewModel.createPlaylistCommand());
    bindButton(*m_deletePlaylistButton, m_playlistViewModel.deletePlaylistCommand());
    bindButton(*m_addSongsToPlaylistButton, m_playlistViewModel.addSongsCommand());
    bindButton(*m_playSelectedSongButton, m_playlistViewModel.playSelectedSongCommand());
    bindButton(*m_playVisiblePlaylistButton, m_playlistViewModel.playVisiblePlaylistCommand());

    connect(m_playlistListView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this](const QModelIndex &current) {
                if (!current.isValid()) {
                    return;
                }

                const QString playlistId = current.data(m_playlistViewModel.playlistIdRole()).toString();
                if (playlistId.isEmpty()) {
                    return;
                }

                m_playlistViewModel.setSelectedPlaylistId(playlistId);
                if (m_playlistViewModel.switchPlaylistCommand() != nullptr) {
                    m_playlistViewModel.switchPlaylistCommand()->execute();
                }
            });

    connect(m_songListView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this](const QModelIndex &current) {
                m_playlistViewModel.setSelectedSongIndex(current.isValid() ? current.row() : -1);
            });

    connect(&m_playlistViewModel,
            &ViewModel::PlaylistCollectionViewModelProtocol::visiblePlaylistChanged,
            this,
            &MainWindow::updatePlaylistSelection);
    connect(&m_playlistViewModel,
            &ViewModel::PlaylistCollectionViewModelProtocol::statusMessageChanged,
            this,
            &MainWindow::updatePlaylistStatusMessage);
    connect(&m_playlistViewModel,
            &ViewModel::PlaylistCollectionViewModelProtocol::lastErrorChanged,
            this,
            &MainWindow::updatePlaylistLastError);

    updatePlaylistSelection();
    updatePlaylistStatusMessage();
    updatePlaylistLastError();
}

void MainWindow::bindPlaybackViewModel()
{
    bindButton(*m_playButton, m_playbackViewModel.playCommand());
    bindButton(*m_pauseButton, m_playbackViewModel.pauseCommand());
    bindButton(*m_stopButton, m_playbackViewModel.stopCommand());
    bindButton(*m_previousButton, m_playbackViewModel.previousCommand());
    bindButton(*m_nextButton, m_playbackViewModel.nextCommand());
    bindButton(*m_muteButton, m_playbackViewModel.toggleMuteCommand());

    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModelProtocol::playbackStateChanged, this, &MainWindow::updatePlaybackState);
    connect(&m_playbackViewModel,
            &ViewModel::PlaybackViewModelProtocol::currentPlaybackTitleChanged,
            this,
            &MainWindow::updateCurrentPlaybackTrack);
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModelProtocol::positionMsChanged, this, &MainWindow::updatePlaybackPosition);
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModelProtocol::durationMsChanged, this, &MainWindow::updatePlaybackDuration);
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModelProtocol::seekableChanged, this, &MainWindow::updatePlaybackSeekable);
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModelProtocol::volumePercentChanged, this, &MainWindow::updatePlaybackVolume);
    connect(&m_playbackViewModel, &ViewModel::PlaybackViewModelProtocol::mutedChanged, this, &MainWindow::updatePlaybackMuted);
    connect(&m_playbackViewModel,
            &ViewModel::PlaybackViewModelProtocol::lastPlaybackErrorChanged,
            this,
            &MainWindow::updatePlaybackError);
    connect(&m_playbackViewModel,
            &ViewModel::PlaybackViewModelProtocol::statusMessageChanged,
            this,
            &MainWindow::updatePlaybackStatusMessage);

    connect(m_progressSlider, &QSlider::sliderPressed, this, [this]() {
        m_userSeeking = true;
    });
    connect(m_progressSlider, &QSlider::sliderReleased, this, [this]() {
        m_userSeeking = false;
        m_playbackViewModel.seekTo(m_progressSlider->value());
        updatePlaybackPosition();
    });
    connect(m_progressSlider, &QSlider::sliderMoved, this, [this](int value) {
        m_playbackPositionLabel->setText(tr("%1 / %2").arg(formatTime(value), formatTime(m_playbackViewModel.durationMs())));
    });

    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!m_syncingVolume) {
            m_playbackViewModel.setVolumePercent(value);
        }
    });

    updatePlaybackState();
    updateCurrentPlaybackTrack();
    updatePlaybackDuration();
    updatePlaybackPosition();
    updatePlaybackSeekable();
    updatePlaybackVolume();
    updatePlaybackMuted();
    updatePlaybackError();
    updatePlaybackStatusMessage();
}


void MainWindow::bindAudioEffectsViewModel()
{
    m_playbackRateComboBox->clear();
    for (const QString &label : m_audioEffectsViewModel.playbackRateLabels()) {
        m_playbackRateComboBox->addItem(label, playbackRateFromLabel(label));
    }

    m_equalizerPresetComboBox->clear();
    m_equalizerPresetComboBox->addItems(m_audioEffectsViewModel.equalizerPresetLabels());

    bindButton(*m_resetPlaybackRateButton, m_audioEffectsViewModel.resetPlaybackRateCommand());
    bindButton(*m_resetEqualizerButton, m_audioEffectsViewModel.resetEqualizerCommand());

    connect(m_playbackRateComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int index) {
                if (m_syncingAudioEffects || index < 0) {
                    return;
                }
                m_audioEffectsViewModel.setPlaybackRate(m_playbackRateComboBox->itemData(index).toDouble());
            });

    connect(m_equalizerEnabledCheckBox, &QCheckBox::toggled, this, [this](bool enabled) {
        if (!m_syncingAudioEffects) {
            m_audioEffectsViewModel.setEqualizerEnabled(enabled);
        }
    });

    for (int index = 0; index < static_cast<int>(m_equalizerSliders.size()); ++index) {
        QSlider *slider = m_equalizerSliders[static_cast<std::size_t>(index)];
        connect(slider, &QSlider::valueChanged, this, [this, index](int value) {
            if (!m_syncingAudioEffects) {
                m_audioEffectsViewModel.setEqualizerBandGain(index, gainFromSliderValue(value));
            }
        });
    }

    connect(m_equalizerPresetComboBox, &QComboBox::currentTextChanged, this, [this](const QString &preset) {
        if (!m_syncingAudioEffects) {
            m_audioEffectsViewModel.setEqualizerPreset(preset);
        }
    });

    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::capabilitiesChanged,
            this,
            &MainWindow::updateAudioEffectsCapabilities);
    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::playbackRateChanged,
            this,
            &MainWindow::updateAudioEffectsPlaybackRate);
    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::equalizerEnabledChanged,
            this,
            &MainWindow::updateAudioEffectsEqualizerEnabled);
    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::equalizerBandGainsChanged,
            this,
            &MainWindow::updateAudioEffectsBandGains);
    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::equalizerPresetChanged,
            this,
            &MainWindow::updateAudioEffectsPreset);
    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::statusMessageChanged,
            this,
            &MainWindow::updateAudioEffectsStatusMessage);
    connect(&m_audioEffectsViewModel,
            &ViewModel::AudioEffectsViewModelProtocol::lastErrorChanged,
            this,
            &MainWindow::updateAudioEffectsLastError);

    updateAudioEffectsCapabilities();
    updateAudioEffectsPlaybackRate();
    updateAudioEffectsEqualizerEnabled();
    updateAudioEffectsBandGains();
    updateAudioEffectsPreset();
    updateAudioEffectsStatusMessage();
    updateAudioEffectsLastError();
}

void MainWindow::bindProcessingViewModel()
{
    bindButton(*m_enqueueProcessingButton, m_processingViewModel.enqueueCommand());
    bindButton(*m_cancelSelectedProcessingButton, m_processingViewModel.cancelSelectedCommand());
    bindButton(*m_cancelAllProcessingButton, m_processingViewModel.cancelAllCommand());

    connect(m_selectProcessingInputsButton, &QPushButton::clicked, this, [this]() {
        const QStringList files = QFileDialog::getOpenFileNames(this, tr("Select audio files to transcode"));
        if (!files.isEmpty()) {
            m_processingViewModel.setInputFilePaths(files);
        }
    });
    connect(m_selectProcessingOutputDirectoryButton, &QPushButton::clicked, this, [this]() {
        const QString directory = QFileDialog::getExistingDirectory(this, tr("Select transcode output directory"));
        if (!directory.isEmpty()) {
            m_processingViewModel.setOutputDirectory(directory);
        }
    });
    connect(m_processingFormatComboBox, &QComboBox::currentTextChanged, this, [this](const QString &format) {
        m_processingViewModel.setOutputFormat(format);
    });
    connect(m_processingTaskListView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this](const QModelIndex &current) {
                m_processingViewModel.setSelectedTaskRow(current.isValid() ? current.row() : -1);
            });

    connect(&m_processingViewModel,
            &ViewModel::ProcessingViewModelProtocol::inputFilePathsChanged,
            this,
            &MainWindow::updateProcessingInputSummary);
    connect(&m_processingViewModel,
            &ViewModel::ProcessingViewModelProtocol::outputDirectoryChanged,
            this,
            &MainWindow::updateProcessingOutputDirectory);
    connect(&m_processingViewModel,
            &ViewModel::ProcessingViewModelProtocol::outputFormatChanged,
            this,
            &MainWindow::updateProcessingOutputFormat);
    connect(&m_processingViewModel,
            &ViewModel::ProcessingViewModelProtocol::hasPendingOrRunningTasksChanged,
            this,
            &MainWindow::updateProcessingActiveState);
    connect(&m_processingViewModel,
            &ViewModel::ProcessingViewModelProtocol::statusMessageChanged,
            this,
            &MainWindow::updateProcessingStatusMessage);
    connect(&m_processingViewModel,
            &ViewModel::ProcessingViewModelProtocol::lastErrorChanged,
            this,
            &MainWindow::updateProcessingLastError);

    updateProcessingInputSummary();
    updateProcessingOutputDirectory();
    updateProcessingOutputFormat();
    updateProcessingActiveState();
    updateProcessingStatusMessage();
    updateProcessingLastError();
}

void MainWindow::bindLineEdit(QLineEdit &lineEdit,
                              const QString &(ViewModel::LibraryViewModelProtocol::*getter)() const noexcept,
                              void (ViewModel::LibraryViewModelProtocol::*setter)(QString),
                              void (ViewModel::LibraryViewModelProtocol::*changedSignal)())
{
    connect(&lineEdit, &QLineEdit::textChanged, this, [this, setter](const QString &text) {
        (m_viewModel.*setter)(text);
    });

    connect(&m_viewModel, changedSignal, this, [this, &lineEdit, getter]() {
        const QString &value = (m_viewModel.*getter)();
        if (lineEdit.text() != value) {
            lineEdit.setText(value);
        }
    });
}

void MainWindow::bindButton(QPushButton &button, Common::ViewCommand *command)
{
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

void MainWindow::updateCount()
{
    m_countLabel->setText(tr("%n song(s)", nullptr, m_viewModel.count()));
}

void MainWindow::updateStatusMessage()
{
    setLabelVisibleText(*m_statusLabel, m_viewModel.statusMessage());
}

void MainWindow::updateLastError()
{
    setLabelVisibleText(*m_errorLabel, m_viewModel.lastError());
}

void MainWindow::updateWarnings()
{
    const QStringList warnings = m_viewModel.warnings();
    QStringList lines;
    lines.reserve(warnings.size());
    for (const QString &warning : warnings) {
        lines.append(tr("Warning: %1").arg(warning));
    }

    setLabelVisibleText(*m_warningsLabel, lines.join(QLatin1Char('\n')));
}

void MainWindow::updatePlaylistSelection()
{
    if (m_playlistListView->model() == nullptr) {
        return;
    }

    const QString visiblePlaylistId = m_playlistViewModel.visiblePlaylistId();
    for (int row = 0; row < m_playlistListView->model()->rowCount(); ++row) {
        const QModelIndex index = m_playlistListView->model()->index(row, 0);
        if (index.data(m_playlistViewModel.playlistIdRole()).toString() == visiblePlaylistId) {
            if (m_playlistListView->currentIndex() != index) {
                m_playlistListView->setCurrentIndex(index);
            }
            return;
        }
    }
}

void MainWindow::updatePlaylistStatusMessage()
{
    const QString status = m_playlistViewModel.statusMessage();
    setLabelVisibleText(*m_playlistStatusLabel, status.isEmpty() ? QString() : tr("Playlist: %1").arg(status));
}

void MainWindow::updatePlaylistLastError()
{
    const QString error = m_playlistViewModel.lastError();
    setLabelVisibleText(*m_playlistErrorLabel, error.isEmpty() ? QString() : tr("Playlist error: %1").arg(error));
}

void MainWindow::updatePlaybackState()
{
    m_playbackStateLabel->setText(tr("Playback state: %1").arg(playbackStateText()));
}

void MainWindow::updateCurrentPlaybackTrack()
{
    const QString title = m_playbackViewModel.currentPlaybackTitle();
    setLabelVisibleText(*m_currentPlaybackLabel, title.isEmpty() ? QString() : tr("Now playing: %1").arg(title));
}

void MainWindow::updatePlaybackPosition()
{
    const int value = sliderValueFromMs(m_playbackViewModel.positionMs());
    if (!m_userSeeking && m_progressSlider->value() != value) {
        m_progressSlider->setValue(value);
    }

    const int displayValue = m_userSeeking ? m_progressSlider->value() : value;
    m_playbackPositionLabel->setText(tr("%1 / %2").arg(formatTime(displayValue), formatTime(m_playbackViewModel.durationMs())));
}

void MainWindow::updatePlaybackDuration()
{
    const int duration = sliderValueFromMs(m_playbackViewModel.durationMs());
    m_progressSlider->setRange(0, duration);
    updatePlaybackPosition();
}

void MainWindow::updatePlaybackSeekable()
{
    m_progressSlider->setEnabled(m_playbackViewModel.seekable() && m_playbackViewModel.durationMs() > 0);
}

void MainWindow::updatePlaybackVolume()
{
    const QSignalBlocker blocker(m_volumeSlider);
    Q_UNUSED(blocker)
    m_syncingVolume = true;
    m_volumeSlider->setValue(m_playbackViewModel.volumePercent());
    m_syncingVolume = false;
}

void MainWindow::updatePlaybackMuted()
{
    m_muteButton->setText(m_playbackViewModel.muted() ? tr("Unmute") : tr("Mute"));
}

void MainWindow::updatePlaybackError()
{
    setLabelVisibleText(*m_playbackErrorLabel, m_playbackViewModel.lastPlaybackError());
}

void MainWindow::updatePlaybackStatusMessage()
{
    setLabelVisibleText(*m_playbackStatusLabel, m_playbackViewModel.statusMessage());
}


void MainWindow::updateAudioEffectsCapabilities()
{
    const bool rateAvailable = m_audioEffectsViewModel.supportsPitchPreservingRate();
    const bool eqAvailable = m_audioEffectsViewModel.supportsEqualizer();
    m_playbackRateComboBox->setEnabled(rateAvailable);
    m_resetPlaybackRateButton->setEnabled(rateAvailable && m_audioEffectsViewModel.resetPlaybackRateCommand() != nullptr);
    m_equalizerEnabledCheckBox->setEnabled(eqAvailable);
    m_equalizerPresetComboBox->setEnabled(eqAvailable);
    m_resetEqualizerButton->setEnabled(eqAvailable && m_audioEffectsViewModel.resetEqualizerCommand() != nullptr);
    for (QSlider *slider : m_equalizerSliders) {
        if (slider != nullptr) {
            slider->setEnabled(eqAvailable);
        }
    }
}

void MainWindow::updateAudioEffectsPlaybackRate()
{
    const QSignalBlocker blocker(m_playbackRateComboBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    const double rate = m_audioEffectsViewModel.playbackRate();
    for (int index = 0; index < m_playbackRateComboBox->count(); ++index) {
        if (std::abs(m_playbackRateComboBox->itemData(index).toDouble() - rate) < 0.001) {
            m_playbackRateComboBox->setCurrentIndex(index);
            break;
        }
    }
    m_syncingAudioEffects = false;
}

void MainWindow::updateAudioEffectsEqualizerEnabled()
{
    const QSignalBlocker blocker(m_equalizerEnabledCheckBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    m_equalizerEnabledCheckBox->setChecked(m_audioEffectsViewModel.equalizerEnabled());
    m_syncingAudioEffects = false;
}

void MainWindow::updateAudioEffectsBandGains()
{
    const std::array<double, 5> gains = {m_audioEffectsViewModel.band0GainDb(),
                                         m_audioEffectsViewModel.band1GainDb(),
                                         m_audioEffectsViewModel.band2GainDb(),
                                         m_audioEffectsViewModel.band3GainDb(),
                                         m_audioEffectsViewModel.band4GainDb()};
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

void MainWindow::updateAudioEffectsPreset()
{
    const QSignalBlocker blocker(m_equalizerPresetComboBox);
    Q_UNUSED(blocker)
    m_syncingAudioEffects = true;
    const int index = m_equalizerPresetComboBox->findText(m_audioEffectsViewModel.equalizerPreset());
    if (index >= 0) {
        m_equalizerPresetComboBox->setCurrentIndex(index);
    }
    m_syncingAudioEffects = false;
}

void MainWindow::updateAudioEffectsStatusMessage()
{
    const QString status = m_audioEffectsViewModel.statusMessage();
    setLabelVisibleText(*m_audioEffectsStatusLabel, status.isEmpty() ? QString() : tr("Effects: %1").arg(status));
}

void MainWindow::updateAudioEffectsLastError()
{
    const QString error = m_audioEffectsViewModel.lastError();
    setLabelVisibleText(*m_audioEffectsErrorLabel, error.isEmpty() ? QString() : tr("Effects error: %1").arg(error));
}

void MainWindow::updateProcessingInputSummary()
{
    const QStringList files = m_processingViewModel.inputFilePaths();
    setLabelVisibleText(*m_processingInputSummaryLabel,
                        files.isEmpty() ? QString() : tr("Transcode inputs: %n file(s)", nullptr, files.size()));
}

void MainWindow::updateProcessingOutputDirectory()
{
    const QString directory = m_processingViewModel.outputDirectory();
    setLabelVisibleText(*m_processingOutputDirectoryLabel,
                        directory.isEmpty() ? QString() : tr("Transcode output: %1").arg(directory));
}

void MainWindow::updateProcessingOutputFormat()
{
    const QString format = m_processingViewModel.outputFormat();
    if (m_processingFormatComboBox->currentText() != format) {
        const int index = m_processingFormatComboBox->findText(format);
        if (index >= 0) {
            m_processingFormatComboBox->setCurrentIndex(index);
        }
    }
}

void MainWindow::updateProcessingActiveState()
{
    m_cancelAllProcessingButton->setEnabled(m_processingViewModel.hasPendingOrRunningTasks());
}

void MainWindow::updateProcessingStatusMessage()
{
    const QString status = m_processingViewModel.statusMessage();
    setLabelVisibleText(*m_processingStatusLabel, status.isEmpty() ? QString() : tr("Processing: %1").arg(status));
}

void MainWindow::updateProcessingLastError()
{
    const QString error = m_processingViewModel.lastError();
    setLabelVisibleText(*m_processingErrorLabel, error.isEmpty() ? QString() : tr("Processing error: %1").arg(error));
}

void MainWindow::setLabelVisibleText(QLabel &label, const QString &text)
{
    label.setText(text);
    label.setVisible(!text.isEmpty());
}

QString MainWindow::playbackStateText() const
{
    switch (m_playbackViewModel.playbackState()) {
    case ViewModel::PlaybackState::Stopped:
        return tr("Stopped");
    case ViewModel::PlaybackState::Loading:
        return tr("Loading");
    case ViewModel::PlaybackState::Playing:
        return tr("Playing");
    case ViewModel::PlaybackState::Paused:
        return tr("Paused");
    case ViewModel::PlaybackState::Buffering:
        return tr("Buffering");
    case ViewModel::PlaybackState::Error:
        return tr("Error");
    }

    return tr("Stopped");
}

int MainWindow::sliderValueFromMs(qint64 valueMs)
{
    const qint64 clamped = std::clamp(valueMs, qint64{0}, static_cast<qint64>(std::numeric_limits<int>::max()));
    return static_cast<int>(clamped);
}

} // namespace AudioPlayer::View
