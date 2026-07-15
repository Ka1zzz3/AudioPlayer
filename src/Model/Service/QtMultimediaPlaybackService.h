#pragma once

#include "Model/Service/IPlaybackService.h"

#include <QMediaPlayer>
#include <QUrl>

#include <memory>

class QAudioOutput;

namespace AudioPlayer::Model::Service {

class QtMultimediaPlaybackService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit QtMultimediaPlaybackService(QObject *parent = nullptr);
    ~QtMultimediaPlaybackService() override;

    [[nodiscard]] QString source() const override;
    [[nodiscard]] PlaybackBackendState state() const noexcept override;
    [[nodiscard]] qint64 positionMs() const noexcept override;
    [[nodiscard]] qint64 durationMs() const noexcept override;
    [[nodiscard]] bool seekable() const noexcept override;
    [[nodiscard]] float volume() const noexcept override;
    [[nodiscard]] bool muted() const noexcept override;

public slots:
    void setSource(QString filePath) override;
    void play() override;
    void pause() override;
    void stop() override;
    void seekTo(qint64 positionMs) override;
    void setVolume(float volume) override;
    void setMuted(bool muted) override;

private:
    void setState(PlaybackBackendState state);
    void handlePlaybackStateChanged(QMediaPlayer::PlaybackState state);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void handleError(QMediaPlayer::Error error, const QString &message);
    [[nodiscard]] PlaybackBackendError mapError(QMediaPlayer::Error error) const noexcept;

    std::unique_ptr<QAudioOutput> m_audioOutput;
    std::unique_ptr<QMediaPlayer> m_player;
    QString m_source;
    PlaybackBackendState m_state = PlaybackBackendState::Stopped;
};

} // namespace AudioPlayer::Model::Service
