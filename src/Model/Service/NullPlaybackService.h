#pragma once

#include "Model/Service/IPlaybackService.h"

namespace AudioPlayer::Model::Service {

class NullPlaybackService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit NullPlaybackService(QObject *parent = nullptr);

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

    QString m_source;
    PlaybackBackendState m_state = PlaybackBackendState::Stopped;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_seekable = false;
    float m_volume = 1.0F;
    bool m_muted = false;
};

} // namespace AudioPlayer::Model::Service
