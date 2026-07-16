#include "Model/Service/GStreamerEffectsPlaybackBackend.h"

#include <QUrl>

#include <algorithm>
#include <array>
#include <mutex>
#include <utility>

namespace AudioPlayer::Model::Service {

namespace {

constexpr std::array<double, AudioEffectsEqualizerBandCount> EqFrequenciesHz = {60.0, 230.0, 910.0, 3600.0, 14000.0};
constexpr double EqBandwidth = 1.0;

void ensureGStreamerInitialized()
{
    static std::once_flag once;
    std::call_once(once, [] { gst_init(nullptr, nullptr); });
}

QString messageFromGError(GError *error, const char *fallback)
{
    if (error != nullptr && error->message != nullptr && error->message[0] != '\0') {
        return QString::fromUtf8(error->message);
    }
    return QString::fromUtf8(fallback);
}

qint64 toMs(gint64 gstTime)
{
    if (gstTime <= 0) {
        return 0;
    }
    return static_cast<qint64>(gstTime / GST_MSECOND);
}

GstElement *makeElement(const char *factoryName, const char *name)
{
    return gst_element_factory_make(factoryName, name);
}

} // namespace

GStreamerEffectsBackendCore::GStreamerEffectsBackendCore(QObject *parent)
    : QObject(parent)
{
    ensureGStreamerInitialized();
    initializePipeline();
    m_positionTimer.setInterval(250);
    connect(&m_positionTimer, &QTimer::timeout, this, &GStreamerEffectsBackendCore::refreshPositionAndDuration);
}

GStreamerEffectsBackendCore::~GStreamerEffectsBackendCore()
{
    teardownPipeline();
}

QString GStreamerEffectsBackendCore::source() const
{
    return m_source;
}

PlaybackBackendState GStreamerEffectsBackendCore::state() const noexcept
{
    return m_state;
}

qint64 GStreamerEffectsBackendCore::positionMs() const noexcept
{
    return m_positionMs;
}

qint64 GStreamerEffectsBackendCore::durationMs() const noexcept
{
    return m_durationMs;
}

bool GStreamerEffectsBackendCore::seekable() const noexcept
{
    return m_seekable;
}

float GStreamerEffectsBackendCore::volume() const noexcept
{
    return m_volume;
}

bool GStreamerEffectsBackendCore::muted() const noexcept
{
    return m_muted;
}

AudioEffectsCapabilities GStreamerEffectsBackendCore::capabilities() const
{
    return m_capabilities;
}

double GStreamerEffectsBackendCore::playbackRate() const noexcept
{
    return m_playbackRate;
}

EqualizerSettings GStreamerEffectsBackendCore::equalizerSettings() const
{
    return m_equalizerSettings;
}

QString GStreamerEffectsBackendCore::statusMessage() const
{
    return m_statusMessage;
}

QString GStreamerEffectsBackendCore::lastError() const
{
    return m_lastError;
}

void GStreamerEffectsBackendCore::setSource(QString filePath)
{
    if (m_source == filePath) {
        return;
    }

    m_source = std::move(filePath);
    m_positionMs = 0;
    m_durationMs = 0;
    refreshSeekable();

    if (m_pipeline != nullptr) {
        gst_element_set_state(m_pipeline, GST_STATE_READY);
        const QString uri = m_source.isEmpty() ? QString() : QUrl::fromLocalFile(m_source).toString();
        g_object_set(m_pipeline, "uri", uri.toUtf8().constData(), nullptr);
    }

    emit sourceChanged(m_source);
    emit positionChanged(m_positionMs);
    emit durationChanged(m_durationMs);
    setState(m_source.isEmpty() ? PlaybackBackendState::Stopped : PlaybackBackendState::Loading);
}

void GStreamerEffectsBackendCore::play()
{
    if (m_pipeline == nullptr) {
        emitPlaybackError(PlaybackBackendError::Unknown, QStringLiteral("GStreamer playback pipeline is unavailable."));
        return;
    }
    if (m_source.isEmpty()) {
        emitPlaybackError(PlaybackBackendError::Resource, QStringLiteral("No audio file is selected."));
        return;
    }

    const GstStateChangeReturn result = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    if (result == GST_STATE_CHANGE_FAILURE) {
        emitPlaybackError(PlaybackBackendError::Unknown, QStringLiteral("Failed to start GStreamer playback."));
        return;
    }
    m_positionTimer.start();
}

void GStreamerEffectsBackendCore::pause()
{
    if (m_pipeline == nullptr) {
        return;
    }
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
}

void GStreamerEffectsBackendCore::stop()
{
    if (m_pipeline == nullptr) {
        return;
    }
    gst_element_set_state(m_pipeline, GST_STATE_READY);
    m_positionTimer.stop();
    if (m_positionMs != 0) {
        m_positionMs = 0;
        emit positionChanged(m_positionMs);
    }
    setState(PlaybackBackendState::Stopped);
}

void GStreamerEffectsBackendCore::seekTo(qint64 positionMs)
{
    if (m_pipeline == nullptr) {
        return;
    }

    const gint64 position = static_cast<gint64>(std::max(positionMs, qint64{0})) * GST_MSECOND;
    const gboolean ok = gst_element_seek(m_pipeline,
                                         m_playbackRate,
                                         GST_FORMAT_TIME,
                                         static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                         GST_SEEK_TYPE_SET,
                                         position,
                                         GST_SEEK_TYPE_NONE,
                                         GST_CLOCK_TIME_NONE);
    if (!ok) {
        emitPlaybackError(PlaybackBackendError::Unknown, QStringLiteral("Failed to seek GStreamer playback."));
        return;
    }

    const qint64 normalized = std::max(positionMs, qint64{0});
    if (m_positionMs != normalized) {
        m_positionMs = normalized;
        emit positionChanged(m_positionMs);
    }
}

void GStreamerEffectsBackendCore::setVolume(float volume)
{
    const float clamped = std::clamp(volume, 0.0F, 1.0F);
    if (qFuzzyCompare(m_volume, clamped)) {
        return;
    }

    m_volume = clamped;
    if (m_pipeline != nullptr) {
        g_object_set(m_pipeline, "volume", static_cast<double>(m_volume), nullptr);
    }
    emit volumeChanged(m_volume);
}

void GStreamerEffectsBackendCore::setMuted(bool muted)
{
    if (m_muted == muted) {
        return;
    }

    m_muted = muted;
    if (m_pipeline != nullptr) {
        g_object_set(m_pipeline, "mute", m_muted, nullptr);
    }
    emit mutedChanged(m_muted);
}

void GStreamerEffectsBackendCore::setPlaybackRate(double rate)
{
    if (!isSupportedPlaybackRate(rate)) {
        setError(QStringLiteral("Unsupported playback rate."));
        emit audioEffectsError(m_lastError);
        return;
    }
    if (qFuzzyCompare(m_playbackRate, rate)) {
        return;
    }

    m_playbackRate = rate;
    if (m_pipeline != nullptr && m_state != PlaybackBackendState::Stopped && m_state != PlaybackBackendState::Loading) {
        const bool rateApplied = applyRateSeek();
        Q_UNUSED(rateApplied)
    }
    setStatus(QStringLiteral("GStreamer playback speed set to %1.").arg(displayLabelForPlaybackRate(rate)));
    setError({});
    emit playbackRateChanged(m_playbackRate);
}

void GStreamerEffectsBackendCore::setEqualizerEnabled(bool enabled)
{
    if (m_equalizerSettings.enabled == enabled) {
        return;
    }

    m_equalizerSettings.enabled = enabled;
    applyEqualizerToElements();
    setStatus(enabled ? QStringLiteral("GStreamer equalizer enabled.") : QStringLiteral("GStreamer equalizer disabled."));
    setError({});
    emit equalizerSettingsChanged();
}

void GStreamerEffectsBackendCore::setEqualizerBandGain(int bandIndex, double gainDb)
{
    if (!isValidEqualizerBandIndex(bandIndex)) {
        setError(QStringLiteral("Invalid equalizer band."));
        emit audioEffectsError(m_lastError);
        return;
    }
    if (!isValidEqualizerGain(gainDb)) {
        setError(QStringLiteral("Equalizer gain is out of range."));
        emit audioEffectsError(m_lastError);
        return;
    }

    m_equalizerSettings.gainsDb[static_cast<std::size_t>(bandIndex)] = gainDb;
    m_equalizerSettings.preset = EqualizerPreset::Flat;
    applyEqualizerToElements();
    setStatus(QStringLiteral("GStreamer equalizer band updated."));
    setError({});
    emit equalizerSettingsChanged();
}

void GStreamerEffectsBackendCore::applyEqualizerPreset(EqualizerPreset preset)
{
    m_equalizerSettings.preset = preset;
    m_equalizerSettings.gainsDb = presetGains(preset);
    applyEqualizerToElements();
    setStatus(QStringLiteral("GStreamer equalizer preset set to %1.").arg(displayLabel(preset)));
    setError({});
    emit equalizerSettingsChanged();
}

void GStreamerEffectsBackendCore::resetEqualizer()
{
    m_equalizerSettings = defaultEqualizerSettings();
    applyEqualizerToElements();
    setStatus(QStringLiteral("GStreamer equalizer reset to Flat."));
    setError({});
    emit equalizerSettingsChanged();
}

gboolean GStreamerEffectsBackendCore::handleBusMessage(GstBus *, GstMessage *message, gpointer userData)
{
    auto *self = static_cast<GStreamerEffectsBackendCore *>(userData);
    if (self != nullptr) {
        self->handleMessage(message);
    }
    return G_SOURCE_CONTINUE;
}

void GStreamerEffectsBackendCore::initializePipeline()
{
    m_pipeline = makeElement("playbin", "audioplayer-playbin");
    if (m_pipeline == nullptr) {
        m_capabilities = unavailableAudioEffectsCapabilities(QStringLiteral("GStreamer playbin is unavailable."));
        setError(m_capabilities.unavailableReason);
        emit capabilitiesChanged();
        return;
    }

    m_audioFilterBin = createAudioFilterBin();
    if (m_audioFilterBin == nullptr) {
        m_capabilities = unavailableAudioEffectsCapabilities(QStringLiteral("GStreamer audio effects elements are unavailable."));
        setError(m_capabilities.unavailableReason);
        emit capabilitiesChanged();
        return;
    }

    g_object_set(m_pipeline, "audio-filter", m_audioFilterBin, nullptr);
    g_object_set(m_pipeline, "volume", static_cast<double>(m_volume), "mute", m_muted, nullptr);

    GstBus *bus = gst_element_get_bus(m_pipeline);
    m_busWatchId = gst_bus_add_watch(bus, &GStreamerEffectsBackendCore::handleBusMessage, this);
    gst_object_unref(bus);

    configureEqualizerBands();
    applyEqualizerToElements();
    setStatus(QStringLiteral("GStreamer effects backend ready."));
}

void GStreamerEffectsBackendCore::teardownPipeline()
{
    m_positionTimer.stop();
    if (m_busWatchId != 0) {
        g_source_remove(m_busWatchId);
        m_busWatchId = 0;
    }
    if (m_pipeline != nullptr) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
    m_audioFilterBin = nullptr;
    m_equalizer = nullptr;
}

GstElement *GStreamerEffectsBackendCore::createAudioFilterBin()
{
    GstElement *bin = gst_bin_new("audioplayer-audio-filter");
    GstElement *convertIn = makeElement("audioconvert", "effects-convert-in");
    GstElement *resample = makeElement("audioresample", "effects-resample");
    GstElement *tempo = makeElement("scaletempo", "effects-scaletempo");
    GstElement *equalizer = makeElement("equalizer-nbands", "effects-equalizer");
    GstElement *convertOut = makeElement("audioconvert", "effects-convert-out");

    if (bin == nullptr || convertIn == nullptr || resample == nullptr || tempo == nullptr || equalizer == nullptr || convertOut == nullptr) {
        if (bin != nullptr) {
            gst_object_unref(bin);
        }
        return nullptr;
    }

    g_object_set(equalizer, "num-bands", static_cast<guint>(AudioEffectsEqualizerBandCount), nullptr);
    gst_bin_add_many(GST_BIN(bin), convertIn, resample, tempo, equalizer, convertOut, nullptr);
    if (!gst_element_link_many(convertIn, resample, tempo, equalizer, convertOut, nullptr)) {
        gst_object_unref(bin);
        return nullptr;
    }

    GstPad *sinkPad = gst_element_get_static_pad(convertIn, "sink");
    GstPad *srcPad = gst_element_get_static_pad(convertOut, "src");
    gst_element_add_pad(bin, gst_ghost_pad_new("sink", sinkPad));
    gst_element_add_pad(bin, gst_ghost_pad_new("src", srcPad));
    gst_object_unref(sinkPad);
    gst_object_unref(srcPad);

    m_equalizer = equalizer;
    return bin;
}

void GStreamerEffectsBackendCore::configureEqualizerBands()
{
    if (m_equalizer == nullptr) {
        return;
    }

    for (guint i = 0; i < AudioEffectsEqualizerBandCount; ++i) {
        GObject *band = gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(m_equalizer), i);
        if (band == nullptr) {
            continue;
        }
        g_object_set(band,
                     "freq",
                     EqFrequenciesHz[static_cast<std::size_t>(i)],
                     "bandwidth",
                     EqBandwidth,
                     nullptr);
        g_object_unref(band);
    }
}

void GStreamerEffectsBackendCore::applyEqualizerToElements()
{
    if (m_equalizer == nullptr) {
        return;
    }

    for (guint i = 0; i < AudioEffectsEqualizerBandCount; ++i) {
        GObject *band = gst_child_proxy_get_child_by_index(GST_CHILD_PROXY(m_equalizer), i);
        if (band == nullptr) {
            continue;
        }
        const double configuredGain = m_equalizerSettings.gainsDb[static_cast<std::size_t>(i)];
        g_object_set(band, "gain", m_equalizerSettings.enabled ? configuredGain : 0.0, nullptr);
        g_object_unref(band);
    }
}

bool GStreamerEffectsBackendCore::applyRateSeek()
{
    if (m_pipeline == nullptr) {
        return false;
    }

    gint64 position = 0;
    gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &position);
    const gboolean ok = gst_element_seek(m_pipeline,
                                         m_playbackRate,
                                         GST_FORMAT_TIME,
                                         static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                                         GST_SEEK_TYPE_SET,
                                         position,
                                         GST_SEEK_TYPE_NONE,
                                         GST_CLOCK_TIME_NONE);
    if (!ok) {
        setError(QStringLiteral("Failed to apply GStreamer playback speed."));
        emit audioEffectsError(m_lastError);
    }
    return ok;
}

void GStreamerEffectsBackendCore::refreshPositionAndDuration()
{
    if (m_pipeline == nullptr) {
        return;
    }

    gint64 position = 0;
    if (gst_element_query_position(m_pipeline, GST_FORMAT_TIME, &position)) {
        const qint64 positionMs = toMs(position);
        if (m_positionMs != positionMs) {
            m_positionMs = positionMs;
            emit positionChanged(m_positionMs);
        }
    }

    gint64 duration = 0;
    if (gst_element_query_duration(m_pipeline, GST_FORMAT_TIME, &duration)) {
        const qint64 durationMs = toMs(duration);
        if (m_durationMs != durationMs) {
            m_durationMs = durationMs;
            emit durationChanged(m_durationMs);
        }
    }

    refreshSeekable();
}

void GStreamerEffectsBackendCore::refreshSeekable()
{
    bool seekable = false;
    if (m_pipeline != nullptr) {
        GstQuery *query = gst_query_new_seeking(GST_FORMAT_TIME);
        if (gst_element_query(m_pipeline, query)) {
            gboolean gstSeekable = FALSE;
            gst_query_parse_seeking(query, nullptr, &gstSeekable, nullptr, nullptr);
            seekable = gstSeekable;
        }
        gst_query_unref(query);
    }

    if (m_seekable != seekable) {
        m_seekable = seekable;
        emit seekableChanged(m_seekable);
    }
}

void GStreamerEffectsBackendCore::setState(PlaybackBackendState state)
{
    if (m_state == state) {
        return;
    }
    m_state = state;
    emit stateChanged(m_state);
}

void GStreamerEffectsBackendCore::setStatus(QString message)
{
    if (m_statusMessage == message) {
        return;
    }
    m_statusMessage = std::move(message);
    emit statusMessageChanged(m_statusMessage);
}

void GStreamerEffectsBackendCore::setError(QString message)
{
    if (m_lastError == message) {
        return;
    }
    m_lastError = std::move(message);
}

void GStreamerEffectsBackendCore::emitPlaybackError(PlaybackBackendError error, QString message)
{
    setError(message);
    setState(PlaybackBackendState::Error);
    emit playbackError(error, m_lastError);
}

void GStreamerEffectsBackendCore::handleMessage(GstMessage *message)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_STATE_CHANGED:
        handleStateChanged(message);
        break;
    case GST_MESSAGE_ERROR:
        handleErrorMessage(message);
        break;
    case GST_MESSAGE_EOS:
        m_positionTimer.stop();
        setState(PlaybackBackendState::Ended);
        emit playbackEnded();
        break;
    case GST_MESSAGE_DURATION_CHANGED:
        handleDurationChanged();
        break;
    default:
        break;
    }
}

void GStreamerEffectsBackendCore::handleStateChanged(GstMessage *message)
{
    if (GST_MESSAGE_SRC(message) != GST_OBJECT(m_pipeline)) {
        return;
    }

    GstState oldState = GST_STATE_NULL;
    GstState newState = GST_STATE_NULL;
    GstState pending = GST_STATE_NULL;
    gst_message_parse_state_changed(message, &oldState, &newState, &pending);
    Q_UNUSED(oldState)
    Q_UNUSED(pending)

    switch (newState) {
    case GST_STATE_READY:
        if (m_state != PlaybackBackendState::Error && m_state != PlaybackBackendState::Ended) {
            setState(PlaybackBackendState::Stopped);
        }
        break;
    case GST_STATE_PAUSED:
        refreshPositionAndDuration();
        if (m_state != PlaybackBackendState::Loading) {
            setState(PlaybackBackendState::Paused);
        }
        break;
    case GST_STATE_PLAYING:
        m_positionTimer.start();
        setState(PlaybackBackendState::Playing);
        break;
    case GST_STATE_NULL:
        setState(PlaybackBackendState::Stopped);
        break;
    case GST_STATE_VOID_PENDING:
        break;
    }
}

void GStreamerEffectsBackendCore::handleErrorMessage(GstMessage *message)
{
    GError *error = nullptr;
    gchar *debug = nullptr;
    gst_message_parse_error(message, &error, &debug);
    Q_UNUSED(debug)
    const QString messageText = messageFromGError(error, "GStreamer playback failed.");
    if (error != nullptr) {
        g_error_free(error);
    }
    g_free(debug);

    m_positionTimer.stop();
    emitPlaybackError(PlaybackBackendError::Unknown, messageText);
    emit audioEffectsError(messageText);
}

void GStreamerEffectsBackendCore::handleDurationChanged()
{
    refreshPositionAndDuration();
}

GStreamerPlaybackService::GStreamerPlaybackService(std::shared_ptr<GStreamerEffectsBackendCore> core, QObject *parent)
    : IPlaybackService(parent)
    , m_core(std::move(core))
{
    Q_ASSERT(m_core != nullptr);
    connect(m_core.get(), &GStreamerEffectsBackendCore::sourceChanged, this, &GStreamerPlaybackService::sourceChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::stateChanged, this, &GStreamerPlaybackService::stateChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::positionChanged, this, &GStreamerPlaybackService::positionChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::durationChanged, this, &GStreamerPlaybackService::durationChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::seekableChanged, this, &GStreamerPlaybackService::seekableChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::volumeChanged, this, &GStreamerPlaybackService::volumeChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::mutedChanged, this, &GStreamerPlaybackService::mutedChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::playbackEnded, this, &GStreamerPlaybackService::playbackEnded);
    connect(m_core.get(), &GStreamerEffectsBackendCore::playbackError, this, &GStreamerPlaybackService::playbackError);
}

QString GStreamerPlaybackService::source() const { return m_core->source(); }
PlaybackBackendState GStreamerPlaybackService::state() const noexcept { return m_core->state(); }
qint64 GStreamerPlaybackService::positionMs() const noexcept { return m_core->positionMs(); }
qint64 GStreamerPlaybackService::durationMs() const noexcept { return m_core->durationMs(); }
bool GStreamerPlaybackService::seekable() const noexcept { return m_core->seekable(); }
float GStreamerPlaybackService::volume() const noexcept { return m_core->volume(); }
bool GStreamerPlaybackService::muted() const noexcept { return m_core->muted(); }
void GStreamerPlaybackService::setSource(QString filePath) { m_core->setSource(std::move(filePath)); }
void GStreamerPlaybackService::play() { m_core->play(); }
void GStreamerPlaybackService::pause() { m_core->pause(); }
void GStreamerPlaybackService::stop() { m_core->stop(); }
void GStreamerPlaybackService::seekTo(qint64 positionMs) { m_core->seekTo(positionMs); }
void GStreamerPlaybackService::setVolume(float volume) { m_core->setVolume(volume); }
void GStreamerPlaybackService::setMuted(bool muted) { m_core->setMuted(muted); }

GStreamerAudioEffectsService::GStreamerAudioEffectsService(std::shared_ptr<GStreamerEffectsBackendCore> core, QObject *parent)
    : IAudioEffectsService(parent)
    , m_core(std::move(core))
{
    Q_ASSERT(m_core != nullptr);
    connect(m_core.get(), &GStreamerEffectsBackendCore::capabilitiesChanged, this, &GStreamerAudioEffectsService::capabilitiesChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::playbackRateChanged, this, &GStreamerAudioEffectsService::playbackRateChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::equalizerSettingsChanged, this, &GStreamerAudioEffectsService::equalizerSettingsChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::statusMessageChanged, this, &GStreamerAudioEffectsService::statusMessageChanged);
    connect(m_core.get(), &GStreamerEffectsBackendCore::audioEffectsError, this, &GStreamerAudioEffectsService::audioEffectsError);
}

AudioEffectsCapabilities GStreamerAudioEffectsService::capabilities() const { return m_core->capabilities(); }
double GStreamerAudioEffectsService::playbackRate() const noexcept { return m_core->playbackRate(); }
EqualizerSettings GStreamerAudioEffectsService::equalizerSettings() const { return m_core->equalizerSettings(); }
QString GStreamerAudioEffectsService::statusMessage() const { return m_core->statusMessage(); }
QString GStreamerAudioEffectsService::lastError() const { return m_core->lastError(); }
void GStreamerAudioEffectsService::setPlaybackRate(double rate) { m_core->setPlaybackRate(rate); }
void GStreamerAudioEffectsService::setEqualizerEnabled(bool enabled) { m_core->setEqualizerEnabled(enabled); }
void GStreamerAudioEffectsService::setEqualizerBandGain(int bandIndex, double gainDb) { m_core->setEqualizerBandGain(bandIndex, gainDb); }
void GStreamerAudioEffectsService::applyEqualizerPreset(EqualizerPreset preset) { m_core->applyEqualizerPreset(preset); }
void GStreamerAudioEffectsService::resetEqualizer() { m_core->resetEqualizer(); }

} // namespace AudioPlayer::Model::Service
