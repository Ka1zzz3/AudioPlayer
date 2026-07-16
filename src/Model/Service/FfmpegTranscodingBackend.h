#pragma once

#include "Model/Service/ITranscodingBackend.h"

#include <QProcess>
#include <QStringList>

#include <memory>

namespace AudioPlayer::Model::Service {

struct FfmpegCommand
{
    QString program;
    QStringList arguments;
};

class FfmpegTranscodingBackend final : public ITranscodingBackend
{
    Q_OBJECT

public:
    explicit FfmpegTranscodingBackend(QString ffmpegProgram = QStringLiteral("ffmpeg"),
                                      QString ffprobeProgram = QStringLiteral("ffprobe"),
                                      QObject *parent = nullptr);
    ~FfmpegTranscodingBackend() override;

    [[nodiscard]] bool available() const noexcept override;
    [[nodiscard]] QString unavailableReason() const override;
    [[nodiscard]] bool busy() const noexcept override;
    [[nodiscard]] QString currentTaskId() const override;

    [[nodiscard]] static FfmpegCommand buildFfprobeCommand(const QString &ffprobeProgram, const QString &inputFilePath);
    [[nodiscard]] static FfmpegCommand buildFfmpegCommand(const QString &ffmpegProgram,
                                                          const QString &inputFilePath,
                                                          const QString &outputFilePath,
                                                          AudioPlayer::Model::ProcessingOutputFormat outputFormat);
    [[nodiscard]] static bool parseDurationMs(const QByteArray &ffprobeJson, qint64 *durationMs);
    [[nodiscard]] static bool parseProgressPercent(const QByteArray &progressOutput, qint64 durationMs, int *percent, bool *finished);
    [[nodiscard]] static QString summarizeProcessError(QProcess::ProcessError error);

public slots:
    void startTranscode(AudioPlayer::Model::Service::TranscodingRequest request) override;
    void cancelCurrent() override;

private slots:
    void handleReadyReadStandardOutput();
    void handleReadyReadStandardError();
    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void handleErrorOccurred(QProcess::ProcessError error);

private:
    void failCurrent(QString message, QString technicalDetails);
    void clearProcess();

    QString m_ffmpegProgram;
    QString m_ffprobeProgram;
    QString m_unavailableReason;
    std::unique_ptr<QProcess> m_process;
    TranscodingRequest m_currentRequest;
    QByteArray m_stdoutBuffer;
    QByteArray m_stderrBuffer;
    qint64 m_durationMs = 0;
    bool m_cancelRequested = false;
};

} // namespace AudioPlayer::Model::Service
