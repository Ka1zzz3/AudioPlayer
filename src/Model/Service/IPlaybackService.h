#pragma once

#include "Model/Service/PlaybackTypes.h"

#include <QObject>
#include <QString>

namespace AudioPlayer::Model::Service {

class IPlaybackService : public QObject
{
    Q_OBJECT

public:
    explicit IPlaybackService(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~IPlaybackService() override = default;

    [[nodiscard]] virtual QString source() const = 0;
    [[nodiscard]] virtual PlaybackBackendState state() const noexcept = 0;
    [[nodiscard]] virtual qint64 positionMs() const noexcept = 0;
    [[nodiscard]] virtual qint64 durationMs() const noexcept = 0;
    [[nodiscard]] virtual bool seekable() const noexcept = 0;
    [[nodiscard]] virtual float volume() const noexcept = 0;
    [[nodiscard]] virtual bool muted() const noexcept = 0;

public slots:
    virtual void setSource(QString filePath) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seekTo(qint64 positionMs) = 0;
    virtual void setVolume(float volume) = 0;
    virtual void setMuted(bool muted) = 0;

signals:
    void sourceChanged(const QString &filePath);
    void stateChanged(AudioPlayer::Model::Service::PlaybackBackendState state);
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void seekableChanged(bool seekable);
    void volumeChanged(float volume);
    void mutedChanged(bool muted);
    void playbackEnded();
    void playbackError(AudioPlayer::Model::Service::PlaybackBackendError error, const QString &message);
};

} // namespace AudioPlayer::Model::Service
