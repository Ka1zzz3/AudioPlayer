#pragma once

#include "Model/AudioFile.h"
#include "Model/Service/IPlaybackService.h"

#include <QObject>
#include <QVector>

#include <optional>

namespace AudioPlayer::Model::Service {

class PlaybackUseCase : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackUseCase(IPlaybackService &playbackService, QObject *parent = nullptr);

    [[nodiscard]] int currentIndex() const noexcept;
    [[nodiscard]] const QVector<AudioFile> &queue() const noexcept;
    [[nodiscard]] std::optional<AudioFile> currentTrack() const;
    [[nodiscard]] bool hasQueue() const noexcept;

public slots:
    void setQueue(QVector<AudioFile> queue);
    bool setCurrentIndex(int index);

    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void seekTo(qint64 positionMs);
    void setVolume(float volume);
    void setMuted(bool muted);

signals:
    void queueChanged();
    void currentIndexChanged(int index);
    void currentTrackChanged(int index, const QString &title, const QString &filePath);
    void playbackErrorOccurred(const QString &message);
    void playbackStopped();

private slots:
    void handlePlaybackEnded();
    void handlePlaybackError(PlaybackBackendError error, const QString &message);

private:
    enum class Direction {
        Forward,
        Backward,
    };

    [[nodiscard]] bool isValidIndex(int index) const noexcept;
    [[nodiscard]] bool isPlayableCandidate(const AudioFile &audioFile, QString *errorMessage) const;
    [[nodiscard]] int findByPath(const QString &filePath) const noexcept;

    void setCurrentIndexInternal(int index);
    void emitCurrentTrackChanged();
    void reportError(QString message);
    void stopAtQueueEnd(const QString &message = {});
    bool startAtOrSkip(int startIndex, Direction direction);
    bool startPlaybackAt(int index);

    IPlaybackService &m_playbackService;
    QVector<AudioFile> m_queue;
    int m_currentIndex = -1;
};

} // namespace AudioPlayer::Model::Service
