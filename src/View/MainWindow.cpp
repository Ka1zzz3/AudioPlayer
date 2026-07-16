#include "View/MainWindow.h"

#include "Common/ViewCommand.h"
#include "ViewModel/LibraryViewModelProtocol.h"
#include "ViewModel/PlaybackViewModelProtocol.h"
#include "ViewModel/PlaybackState.h"

#include <QAbstractItemView>
#include <QAbstractItemModel>
#include <QBoxLayout>
#include <QFormLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QStringList>
#include <QWidget>

#include <limits>

namespace AudioPlayer::View {

namespace {
constexpr auto defaultStoragePath = "library.json";

QString formatTime(qint64 milliseconds)
{
    const qint64 totalSeconds = std::max(milliseconds, qint64{0}) / 1000;
    const qint64 minutes = totalSeconds / 60;
    const qint64 seconds = totalSeconds % 60;
    return QStringLiteral("%1:%2").arg(minutes).arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace

MainWindow::MainWindow(ViewModel::LibraryViewModelProtocol &libraryViewModel,
                       ViewModel::PlaybackViewModelProtocol &playbackViewModel,
                       QWidget *parent)
    : QMainWindow(parent)
    , m_viewModel(libraryViewModel)
    , m_playbackViewModel(playbackViewModel)
{
    buildUi();
    bindViewModel();
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

    setCentralWidget(centralWidget);
}

void MainWindow::bindViewModel()
{
    bindLibraryViewModel();
    bindPlaybackViewModel();
}

void MainWindow::bindLibraryViewModel()
{
    m_viewModel.setStoragePath(m_storagePathInput->text());
    m_viewModel.setScanDirectoryPath(m_scanDirectoryPathInput->text());

    bindLineEdit(*m_storagePathInput,
                 &ViewModel::LibraryViewModelProtocol::storagePath,
                 &ViewModel::LibraryViewModelProtocol::setStoragePath,
                 &ViewModel::LibraryViewModelProtocol::storagePathChanged);
    bindLineEdit(*m_scanDirectoryPathInput,
                 &ViewModel::LibraryViewModelProtocol::scanDirectoryPath,
                 &ViewModel::LibraryViewModelProtocol::setScanDirectoryPath,
                 &ViewModel::LibraryViewModelProtocol::scanDirectoryPathChanged);

    bindButton(*m_scanButton, m_viewModel.scanCommand());
    bindButton(*m_saveButton, m_viewModel.saveCommand());
    bindButton(*m_loadButton, m_viewModel.loadCommand());
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
            &ViewModel::PlaybackViewModelProtocol::currentPlaybackIndexChanged,
            this,
            &MainWindow::syncPlaybackSelection);
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
    syncPlaybackSelection();
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

void MainWindow::syncPlaybackSelection()
{
    const int row = m_playbackViewModel.currentPlaybackIndex();
    if (m_songListView->model() == nullptr || row < 0 || row >= m_songListView->model()->rowCount()) {
        return;
    }

    const QModelIndex index = m_songListView->model()->index(row, 0);
    if (!index.isValid() || m_songListView->currentIndex() == index) {
        return;
    }

    m_songListView->setCurrentIndex(index);
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
