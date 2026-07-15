#pragma once

#include "Common/ViewCommand.h"
#include "ViewModel/PlaybackState.h"

#include <QObject>
#include <QString>

namespace AudioPlayer::ViewModel {

class PlaybackViewModelProtocol : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackViewModelProtocol(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] virtual PlaybackState playbackState() const noexcept = 0;
    [[nodiscard]] virtual int currentPlaybackIndex() const noexcept = 0;
    [[nodiscard]] virtual const QString &currentPlaybackTitle() const noexcept = 0;
    [[nodiscard]] virtual qint64 positionMs() const noexcept = 0;
    [[nodiscard]] virtual qint64 durationMs() const noexcept = 0;
    [[nodiscard]] virtual bool seekable() const noexcept = 0;
    [[nodiscard]] virtual int volumePercent() const noexcept = 0;
    [[nodiscard]] virtual bool muted() const noexcept = 0;
    [[nodiscard]] virtual const QString &lastPlaybackError() const noexcept = 0;
    [[nodiscard]] virtual const QString &statusMessage() const noexcept = 0;

    [[nodiscard]] virtual Common::ViewCommand *playCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *pauseCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *stopCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *previousCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *nextCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *toggleMuteCommand() noexcept = 0;

public slots:
    virtual void setCurrentPlaybackIndex(int index) = 0;
    virtual void seekTo(qint64 positionMs) = 0;
    virtual void setVolumePercent(int volumePercent) = 0;
    virtual void setMuted(bool muted) = 0;

signals:
    void playbackStateChanged();
    void currentPlaybackIndexChanged();
    void currentPlaybackTitleChanged();
    void positionMsChanged();
    void durationMsChanged();
    void seekableChanged();
    void volumePercentChanged();
    void mutedChanged();
    void lastPlaybackErrorChanged();
    void statusMessageChanged();
};

} // namespace AudioPlayer::ViewModel
