#pragma once

#include "Model/Service/IAudioEffectsService.h"
#include "Model/Service/IPlaybackService.h"

#include <QObject>
#include <QString>
#include <QTimer>

#include <gst/gst.h>

#include <memory>

namespace AudioPlayer::Model::Service {

class GStreamerEffectsBackendCore final : public QObject
{
    Q_OBJECT

public:
    explicit GStreamerEffectsBackendCore(QObject *parent = nullptr);
    ~GStreamerEffectsBackendCore() override;

    [[nodiscard]] QString source() const;
    [[nodiscard]] PlaybackBackendState state() const noexcept;
    [[nodiscard]] qint64 positionMs() const noexcept;
    [[nodiscard]] qint64 durationMs() const noexcept;
    [[nodiscard]] bool seekable() const noexcept;
    [[nodiscard]] float volume() const noexcept;
    [[nodiscard]] bool muted() const noexcept;

    [[nodiscard]] AudioEffectsCapabilities capabilities() const;
    [[nodiscard]] double playbackRate() const noexcept;
    [[nodiscard]] EqualizerSettings equalizerSettings() const;
    [[nodiscard]] QString statusMessage() const;
    [[nodiscard]] QString lastError() const;

public slots:
    void setSource(QString filePath);
    void play();
    void pause();
    void stop();
    void seekTo(qint64 positionMs);
    void setVolume(float volume);
    void setMuted(bool muted);

    void setPlaybackRate(double rate);
    void setEqualizerEnabled(bool enabled);
    void setEqualizerBandGain(int bandIndex, double gainDb);
    void applyEqualizerPreset(EqualizerPreset preset);
    void resetEqualizer();

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

    void capabilitiesChanged();
    void playbackRateChanged(double rate);
    void equalizerSettingsChanged();
    void statusMessageChanged(const QString &message);
    void audioEffectsError(const QString &message);

private:
    static gboolean handleBusMessage(GstBus *bus, GstMessage *message, gpointer userData);

    void initializePipeline();
    void teardownPipeline();
    [[nodiscard]] GstElement *createAudioFilterBin();
    void configureEqualizerBands();
    void applyEqualizerToElements();
    [[nodiscard]] bool applyRateSeek();
    void refreshPositionAndDuration();
    void refreshSeekable();
    void setState(PlaybackBackendState state);
    void setStatus(QString message);
    void setError(QString message);
    void emitPlaybackError(PlaybackBackendError error, QString message);
    void handleMessage(GstMessage *message);
    void handleStateChanged(GstMessage *message);
    void handleErrorMessage(GstMessage *message);
    void handleDurationChanged();

    QString m_source;
    PlaybackBackendState m_state = PlaybackBackendState::Stopped;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    bool m_seekable = false;
    float m_volume = 1.0F;
    bool m_muted = false;

    AudioEffectsCapabilities m_capabilities = defaultAudioEffectsCapabilities();
    double m_playbackRate = AudioEffectsDefaultPlaybackRate;
    EqualizerSettings m_equalizerSettings = defaultEqualizerSettings();
    QString m_statusMessage;
    QString m_lastError;

    GstElement *m_pipeline = nullptr;
    GstElement *m_audioFilterBin = nullptr;
    GstElement *m_equalizer = nullptr;
    guint m_busWatchId = 0;
    QTimer m_positionTimer;
};

class GStreamerPlaybackService final : public IPlaybackService
{
    Q_OBJECT

public:
    explicit GStreamerPlaybackService(std::shared_ptr<GStreamerEffectsBackendCore> core, QObject *parent = nullptr);

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
    std::shared_ptr<GStreamerEffectsBackendCore> m_core;
};

class GStreamerAudioEffectsService final : public IAudioEffectsService
{
    Q_OBJECT

public:
    explicit GStreamerAudioEffectsService(std::shared_ptr<GStreamerEffectsBackendCore> core, QObject *parent = nullptr);

    [[nodiscard]] AudioEffectsCapabilities capabilities() const override;
    [[nodiscard]] double playbackRate() const noexcept override;
    [[nodiscard]] EqualizerSettings equalizerSettings() const override;
    [[nodiscard]] QString statusMessage() const override;
    [[nodiscard]] QString lastError() const override;

public slots:
    void setPlaybackRate(double rate) override;
    void setEqualizerEnabled(bool enabled) override;
    void setEqualizerBandGain(int bandIndex, double gainDb) override;
    void applyEqualizerPreset(AudioPlayer::Model::Service::EqualizerPreset preset) override;
    void resetEqualizer() override;

private:
    std::shared_ptr<GStreamerEffectsBackendCore> m_core;
};

} // namespace AudioPlayer::Model::Service
