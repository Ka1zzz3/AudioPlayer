#include "Model/Service/QtMultimediaPlaybackService.h"

#include <QAudioOutput>

#include <algorithm>
#include <utility>

namespace AudioPlayer::Model::Service {

QtMultimediaPlaybackService::QtMultimediaPlaybackService(QObject *parent)
    : IPlaybackService(parent)
    , m_audioOutput(std::make_unique<QAudioOutput>())
    , m_player(std::make_unique<QMediaPlayer>())
{
    m_player->setAudioOutput(m_audioOutput.get());

    connect(m_player.get(), &QMediaPlayer::playbackStateChanged, this, &QtMultimediaPlaybackService::handlePlaybackStateChanged);
    connect(m_player.get(), &QMediaPlayer::mediaStatusChanged, this, &QtMultimediaPlaybackService::handleMediaStatusChanged);
    connect(m_player.get(), &QMediaPlayer::positionChanged, this, &QtMultimediaPlaybackService::positionChanged);
    connect(m_player.get(), &QMediaPlayer::durationChanged, this, &QtMultimediaPlaybackService::durationChanged);
    connect(m_player.get(), &QMediaPlayer::seekableChanged, this, &QtMultimediaPlaybackService::seekableChanged);
    connect(m_player.get(), &QMediaPlayer::errorOccurred, this, &QtMultimediaPlaybackService::handleError);
    connect(m_audioOutput.get(), &QAudioOutput::volumeChanged, this, &QtMultimediaPlaybackService::volumeChanged);
    connect(m_audioOutput.get(), &QAudioOutput::mutedChanged, this, &QtMultimediaPlaybackService::mutedChanged);
}

QtMultimediaPlaybackService::~QtMultimediaPlaybackService() = default;

QString QtMultimediaPlaybackService::source() const
{
    return m_source;
}

PlaybackBackendState QtMultimediaPlaybackService::state() const noexcept
{
    return m_state;
}

qint64 QtMultimediaPlaybackService::positionMs() const noexcept
{
    return m_player->position();
}

qint64 QtMultimediaPlaybackService::durationMs() const noexcept
{
    return m_player->duration();
}

bool QtMultimediaPlaybackService::seekable() const noexcept
{
    return m_player->isSeekable();
}

float QtMultimediaPlaybackService::volume() const noexcept
{
    return m_audioOutput->volume();
}

bool QtMultimediaPlaybackService::muted() const noexcept
{
    return m_audioOutput->isMuted();
}

void QtMultimediaPlaybackService::setSource(QString filePath)
{
    if (m_source == filePath) {
        return;
    }

    m_source = std::move(filePath);
    m_player->setSource(m_source.isEmpty() ? QUrl() : QUrl::fromLocalFile(m_source));
    emit sourceChanged(m_source);
}

void QtMultimediaPlaybackService::play()
{
    if (m_source.isEmpty()) {
        handleError(QMediaPlayer::ResourceError, QStringLiteral("No audio file is selected."));
        return;
    }

    m_player->play();
}

void QtMultimediaPlaybackService::pause()
{
    m_player->pause();
}

void QtMultimediaPlaybackService::stop()
{
    m_player->stop();
}

void QtMultimediaPlaybackService::seekTo(qint64 positionMs)
{
    if (!m_player->isSeekable()) {
        return;
    }

    m_player->setPosition(std::max(positionMs, qint64{0}));
}

void QtMultimediaPlaybackService::setVolume(float volume)
{
    m_audioOutput->setVolume(std::clamp(volume, 0.0F, 1.0F));
}

void QtMultimediaPlaybackService::setMuted(bool muted)
{
    m_audioOutput->setMuted(muted);
}

void QtMultimediaPlaybackService::setState(PlaybackBackendState state)
{
    if (m_state == state) {
        return;
    }

    m_state = state;
    emit stateChanged(m_state);
}

void QtMultimediaPlaybackService::handlePlaybackStateChanged(QMediaPlayer::PlaybackState state)
{
    switch (state) {
    case QMediaPlayer::PlayingState:
        setState(PlaybackBackendState::Playing);
        break;
    case QMediaPlayer::PausedState:
        setState(PlaybackBackendState::Paused);
        break;
    case QMediaPlayer::StoppedState:
        if (m_state != PlaybackBackendState::Ended && m_state != PlaybackBackendState::Error) {
            setState(PlaybackBackendState::Stopped);
        }
        break;
    }
}

void QtMultimediaPlaybackService::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch (status) {
    case QMediaPlayer::LoadingMedia:
        setState(PlaybackBackendState::Loading);
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::StalledMedia:
        setState(PlaybackBackendState::Buffering);
        break;
    case QMediaPlayer::EndOfMedia:
        setState(PlaybackBackendState::Ended);
        emit playbackEnded();
        break;
    case QMediaPlayer::InvalidMedia:
        handleError(QMediaPlayer::ResourceError, m_player->errorString());
        break;
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
    case QMediaPlayer::BufferedMedia:
        break;
    }
}

void QtMultimediaPlaybackService::handleError(QMediaPlayer::Error error, const QString &message)
{
    if (error == QMediaPlayer::NoError) {
        return;
    }

    const QString fallbackMessage = QStringLiteral("Playback failed.");
    setState(PlaybackBackendState::Error);
    emit playbackError(mapError(error), message.isEmpty() ? fallbackMessage : message);
}

PlaybackBackendError QtMultimediaPlaybackService::mapError(QMediaPlayer::Error error) const noexcept
{
    switch (error) {
    case QMediaPlayer::NoError:
        return PlaybackBackendError::None;
    case QMediaPlayer::ResourceError:
        return PlaybackBackendError::Resource;
    case QMediaPlayer::FormatError:
        return PlaybackBackendError::Format;
    case QMediaPlayer::NetworkError:
        return PlaybackBackendError::Network;
    case QMediaPlayer::AccessDeniedError:
        return PlaybackBackendError::AccessDenied;
    }

    return PlaybackBackendError::Unknown;
}

} // namespace AudioPlayer::Model::Service
