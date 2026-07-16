#include "Model/Service/FfmpegTranscodingBackend.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace AudioPlayer::Model::Service {

namespace {

QString codecFor(AudioPlayer::Model::ProcessingOutputFormat format)
{
    switch (format) {
    case AudioPlayer::Model::ProcessingOutputFormat::Mp3:
        return QStringLiteral("libmp3lame");
    case AudioPlayer::Model::ProcessingOutputFormat::Wav:
        return QStringLiteral("pcm_s16le");
    case AudioPlayer::Model::ProcessingOutputFormat::Flac:
        return QStringLiteral("flac");
    }
    return {};
}

} // namespace

FfmpegTranscodingBackend::FfmpegTranscodingBackend(QString ffmpegProgram, QString ffprobeProgram, QObject *parent)
    : ITranscodingBackend(parent)
    , m_ffmpegProgram(std::move(ffmpegProgram))
    , m_ffprobeProgram(std::move(ffprobeProgram))
{
    if (m_ffmpegProgram.trimmed().isEmpty()) {
        m_unavailableReason = QStringLiteral("ffmpeg executable was not configured.");
    } else if (m_ffprobeProgram.trimmed().isEmpty()) {
        m_unavailableReason = QStringLiteral("ffprobe executable was not configured.");
    }
}

FfmpegTranscodingBackend::~FfmpegTranscodingBackend()
{
    if (m_process && m_process->state() != QProcess::NotRunning) {
        m_process->kill();
        m_process->waitForFinished(1000);
    }
}

bool FfmpegTranscodingBackend::available() const noexcept
{
    return m_unavailableReason.isEmpty();
}

QString FfmpegTranscodingBackend::unavailableReason() const
{
    return m_unavailableReason;
}

bool FfmpegTranscodingBackend::busy() const noexcept
{
    return m_process && m_process->state() != QProcess::NotRunning;
}

QString FfmpegTranscodingBackend::currentTaskId() const
{
    return m_currentRequest.taskId;
}

FfmpegCommand FfmpegTranscodingBackend::buildFfprobeCommand(const QString &ffprobeProgram, const QString &inputFilePath)
{
    return {ffprobeProgram,
            {QStringLiteral("-v"),
             QStringLiteral("error"),
             QStringLiteral("-show_format"),
             QStringLiteral("-show_streams"),
             QStringLiteral("-of"),
             QStringLiteral("json"),
             inputFilePath}};
}

FfmpegCommand FfmpegTranscodingBackend::buildFfmpegCommand(const QString &ffmpegProgram,
                                                           const QString &inputFilePath,
                                                           const QString &outputFilePath,
                                                           AudioPlayer::Model::ProcessingOutputFormat outputFormat)
{
    QStringList arguments{QStringLiteral("-nostdin"),
                          QStringLiteral("-n"),
                          QStringLiteral("-i"),
                          inputFilePath,
                          QStringLiteral("-vn"),
                          QStringLiteral("-c:a"),
                          codecFor(outputFormat),
                          QStringLiteral("-progress"),
                          QStringLiteral("pipe:1"),
                          QStringLiteral("-stats_period"),
                          QStringLiteral("0.5"),
                          outputFilePath};
    return {ffmpegProgram, arguments};
}

bool FfmpegTranscodingBackend::parseDurationMs(const QByteArray &ffprobeJson, qint64 *durationMs)
{
    if (durationMs == nullptr) {
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(ffprobeJson);
    if (!document.isObject()) {
        return false;
    }

    bool ok = false;
    const double seconds = document.object().value(QStringLiteral("format")).toObject().value(QStringLiteral("duration")).toString().toDouble(&ok);
    if (!ok || seconds <= 0.0) {
        return false;
    }

    *durationMs = static_cast<qint64>(seconds * 1000.0);
    return true;
}

bool FfmpegTranscodingBackend::parseProgressPercent(const QByteArray &progressOutput, qint64 durationMs, int *percent, bool *finished)
{
    if (percent == nullptr || finished == nullptr) {
        return false;
    }

    bool foundTime = false;
    *finished = false;
    qint64 outTimeMs = 0;
    const QList<QByteArray> lines = progressOutput.split('\n');
    for (QByteArray line : lines) {
        line = line.trimmed();
        if (line.startsWith("out_time_ms=")) {
            bool ok = false;
            const qint64 value = line.mid(QByteArray("out_time_ms=").size()).toLongLong(&ok);
            if (ok) {
                outTimeMs = value / 1000;
                foundTime = true;
            }
        } else if (line.startsWith("out_time_us=")) {
            bool ok = false;
            const qint64 value = line.mid(QByteArray("out_time_us=").size()).toLongLong(&ok);
            if (ok) {
                outTimeMs = value / 1000;
                foundTime = true;
            }
        } else if (line == "progress=end") {
            *finished = true;
        }
    }

    if (*finished) {
        *percent = 100;
        return true;
    }
    if (!foundTime || durationMs <= 0) {
        return false;
    }

    *percent = std::clamp(static_cast<int>((outTimeMs * 100) / durationMs), 0, 100);
    return true;
}

QString FfmpegTranscodingBackend::summarizeProcessError(QProcess::ProcessError error)
{
    switch (error) {
    case QProcess::FailedToStart:
        return QStringLiteral("Failed to start ffmpeg process.");
    case QProcess::Crashed:
        return QStringLiteral("ffmpeg process crashed.");
    case QProcess::Timedout:
        return QStringLiteral("ffmpeg process timed out.");
    case QProcess::WriteError:
        return QStringLiteral("Failed to write to ffmpeg process.");
    case QProcess::ReadError:
        return QStringLiteral("Failed to read from ffmpeg process.");
    case QProcess::UnknownError:
        return QStringLiteral("Unknown ffmpeg process error.");
    }
    return QStringLiteral("Unknown ffmpeg process error.");
}

void FfmpegTranscodingBackend::startTranscode(TranscodingRequest request)
{
    if (!available()) {
        emit transcodeFailed(TranscodingError{request.taskId, QStringLiteral("Transcoding backend unavailable"), m_unavailableReason});
        return;
    }
    if (busy()) {
        emit transcodeFailed(TranscodingError{request.taskId, QStringLiteral("Transcoding backend is busy"), {}});
        return;
    }
    if (!QFileInfo::exists(request.inputFilePath)) {
        emit transcodeFailed(TranscodingError{request.taskId, QStringLiteral("Input file does not exist"), request.inputFilePath});
        return;
    }

    m_currentRequest = std::move(request);
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_durationMs = 0;
    m_cancelRequested = false;

    QProcess ffprobe;
    const FfmpegCommand probeCommand = buildFfprobeCommand(m_ffprobeProgram, m_currentRequest.inputFilePath);
    ffprobe.start(probeCommand.program, probeCommand.arguments);
    if (ffprobe.waitForFinished(5000)) {
        static_cast<void>(parseDurationMs(ffprobe.readAllStandardOutput(), &m_durationMs));
    }

    m_process = std::make_unique<QProcess>();
    connect(m_process.get(), &QProcess::readyReadStandardOutput, this, &FfmpegTranscodingBackend::handleReadyReadStandardOutput);
    connect(m_process.get(), &QProcess::readyReadStandardError, this, &FfmpegTranscodingBackend::handleReadyReadStandardError);
    connect(m_process.get(), qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &FfmpegTranscodingBackend::handleFinished);
    connect(m_process.get(), &QProcess::errorOccurred, this, &FfmpegTranscodingBackend::handleErrorOccurred);

    const FfmpegCommand ffmpegCommand = buildFfmpegCommand(m_ffmpegProgram,
                                                           m_currentRequest.inputFilePath,
                                                           m_currentRequest.outputFilePath,
                                                           m_currentRequest.outputFormat);
    m_process->start(ffmpegCommand.program, ffmpegCommand.arguments);
}

void FfmpegTranscodingBackend::cancelCurrent()
{
    if (!busy()) {
        return;
    }

    m_cancelRequested = true;
    m_process->terminate();
    if (!m_process->waitForFinished(1000)) {
        m_process->kill();
    }
}

void FfmpegTranscodingBackend::handleReadyReadStandardOutput()
{
    if (!m_process) {
        return;
    }

    m_stdoutBuffer += m_process->readAllStandardOutput();
    int percent = 0;
    bool finished = false;
    if (parseProgressPercent(m_stdoutBuffer, m_durationMs, &percent, &finished)) {
        emit progressChanged(TranscodingProgress{m_currentRequest.taskId, percent, false});
        if (finished) {
            m_stdoutBuffer.clear();
        }
    } else {
        emit progressChanged(TranscodingProgress{m_currentRequest.taskId, 0, true});
    }
}

void FfmpegTranscodingBackend::handleReadyReadStandardError()
{
    if (!m_process) {
        return;
    }
    m_stderrBuffer += m_process->readAllStandardError();
}

void FfmpegTranscodingBackend::handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    const QString taskId = m_currentRequest.taskId;
    const QString outputPath = m_currentRequest.outputFilePath;
    const QString technicalDetails = QString::fromUtf8(m_stderrBuffer).trimmed();

    clearProcess();

    if (m_cancelRequested) {
        emit transcodeCanceled(taskId);
        return;
    }
    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        emit transcodeSucceeded(TranscodingResult{taskId, outputPath});
        return;
    }

    emit transcodeFailed(TranscodingError{taskId,
                                          QStringLiteral("Transcode failed"),
                                          technicalDetails.isEmpty() ? QStringLiteral("ffmpeg exited with code %1").arg(exitCode) : technicalDetails});
}

void FfmpegTranscodingBackend::handleErrorOccurred(QProcess::ProcessError error)
{
    if (m_cancelRequested) {
        return;
    }
    failCurrent(summarizeProcessError(error), QString::fromUtf8(m_stderrBuffer).trimmed());
}

void FfmpegTranscodingBackend::failCurrent(QString message, QString technicalDetails)
{
    const QString taskId = m_currentRequest.taskId;
    clearProcess();
    emit transcodeFailed(TranscodingError{taskId, std::move(message), std::move(technicalDetails)});
}

void FfmpegTranscodingBackend::clearProcess()
{
    if (m_process) {
        m_process->disconnect(this);
        m_process.reset();
    }
    m_currentRequest = {};
    m_stdoutBuffer.clear();
    m_stderrBuffer.clear();
    m_durationMs = 0;
    m_cancelRequested = false;
}

} // namespace AudioPlayer::Model::Service
