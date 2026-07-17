#pragma once

#include "ViewModel/PlaybackViewModelProtocol.h"

#include <QObject>
#include <QVector>

namespace AudioPlayer::Model {
class AudioFile;
}

namespace AudioPlayer::Model::Service {
class PlaybackUseCase;
enum class PlaybackBackendError;
enum class PlaybackBackendState;
}

namespace AudioPlayer::ViewModel {

namespace ModelService = AudioPlayer::Model::Service;

class PlaybackViewModel : public PlaybackViewModelProtocol
{
    Q_OBJECT
    Q_PROPERTY(AudioPlayer::ViewModel::PlaybackState playbackState READ playbackState NOTIFY playbackStateChanged)
    Q_PROPERTY(int currentPlaybackIndex READ currentPlaybackIndex NOTIFY currentPlaybackIndexChanged)
    Q_PROPERTY(QString currentPlaybackTitle READ currentPlaybackTitle NOTIFY currentPlaybackTitleChanged)
    Q_PROPERTY(qint64 positionMs READ positionMs NOTIFY positionMsChanged)
    Q_PROPERTY(qint64 durationMs READ durationMs NOTIFY durationMsChanged)
    Q_PROPERTY(bool seekable READ seekable NOTIFY seekableChanged)
    Q_PROPERTY(int volumePercent READ volumePercent WRITE setVolumePercent NOTIFY volumePercentChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY mutedChanged)
    Q_PROPERTY(QString lastPlaybackError READ lastPlaybackError NOTIFY lastPlaybackErrorChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *playCommand READ playCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *pauseCommand READ pauseCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *stopCommand READ stopCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *previousCommand READ previousCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *nextCommand READ nextCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *toggleMuteCommand READ toggleMuteCommand CONSTANT)

public:
    explicit PlaybackViewModel(ModelService::PlaybackUseCase &playbackUseCase, QObject *parent = nullptr);

    [[nodiscard]] PlaybackState playbackState() const noexcept override;
    [[nodiscard]] int currentPlaybackIndex() const noexcept override;
    [[nodiscard]] const QString &currentPlaybackTitle() const noexcept override;
    [[nodiscard]] qint64 positionMs() const noexcept override;
    [[nodiscard]] qint64 durationMs() const noexcept override;
    [[nodiscard]] bool seekable() const noexcept override;
    [[nodiscard]] int volumePercent() const noexcept override;
    [[nodiscard]] bool muted() const noexcept override;
    [[nodiscard]] const QString &lastPlaybackError() const noexcept override;
    [[nodiscard]] const QString &statusMessage() const noexcept override;

    [[nodiscard]] Common::ViewCommand *playCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *pauseCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *stopCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *previousCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *nextCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *toggleMuteCommand() noexcept override;

    void replaceQueue(QVector<Model::AudioFile> queue);

public slots:
    void setCurrentPlaybackIndex(int index) override;
    void seekTo(qint64 positionMs) override;
    void setVolumePercent(int volumePercent) override;
    void setMuted(bool muted) override;

private:
    bool executePlay();
    bool executePause();
    bool executeStop();
    bool executePrevious();
    bool executeNext();
    bool executeToggleMute();

    void updatePlaybackState(ModelService::PlaybackBackendState state);
    void updateCurrentTrack(int index, const QString &title, const QString &filePath);
    void updatePosition(qint64 positionMs);
    void updateDuration(qint64 durationMs);
    void updateSeekable(bool seekable);
    void updateVolume(float volume);
    void updateMuted(bool muted);
    void updatePlaybackError(const QString &message);
    void updateStatusMessage(QString statusMessage);
    void clearPlaybackError();

    ModelService::PlaybackUseCase &m_playbackUseCase;
    Common::ViewCommand m_playCommand;
    Common::ViewCommand m_pauseCommand;
    Common::ViewCommand m_stopCommand;
    Common::ViewCommand m_previousCommand;
    Common::ViewCommand m_nextCommand;
    Common::ViewCommand m_toggleMuteCommand;
    PlaybackState m_playbackState = PlaybackState::Stopped;
    int m_currentPlaybackIndex = -1;
    QString m_currentPlaybackTitle;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_seekable = false;
    int m_volumePercent = 100;
    bool m_muted = false;
    QString m_lastPlaybackError;
    QString m_statusMessage;
};

} // namespace AudioPlayer::ViewModel
