#pragma once

#include "Model/ProcessingTask.h"
#include "Model/Service/ITranscodingBackend.h"

#include <QDateTime>
#include <QDir>
#include <QObject>
#include <QSet>
#include <QVector>

#include <memory>

namespace AudioPlayer::Model::Service {

struct ProcessingEnqueueRequest
{
    QVector<QString> inputFilePaths;
    QString outputDirectory;
    AudioPlayer::Model::ProcessingOutputFormat outputFormat = AudioPlayer::Model::ProcessingOutputFormat::Mp3;
    QDateTime timestamp;
};

struct ProcessingEnqueueResult
{
    QVector<AudioPlayer::Model::ProcessingTask> tasks;
    QString status;
    QString error;

    [[nodiscard]] bool ok() const noexcept { return error.isEmpty(); }
};

class ProcessingUseCase : public QObject
{
    Q_OBJECT

public:
    explicit ProcessingUseCase(ITranscodingBackend *backend, QObject *parent = nullptr);

    [[nodiscard]] const QVector<AudioPlayer::Model::ProcessingTask> &tasks() const noexcept;
    [[nodiscard]] bool hasPendingOrRunningTasks() const noexcept;
    [[nodiscard]] bool running() const noexcept;

    [[nodiscard]] ProcessingEnqueueResult enqueueBatch(const ProcessingEnqueueRequest &request);
    [[nodiscard]] bool cancelTask(const QString &taskId, const QDateTime &timestamp);
    void cancelAll(const QDateTime &timestamp);

signals:
    void tasksChanged();
    void taskUpdated(const QString &taskId);
    void queueDrained();

private slots:
    void handleProgress(AudioPlayer::Model::Service::TranscodingProgress progress);
    void handleSucceeded(AudioPlayer::Model::Service::TranscodingResult result);
    void handleFailed(AudioPlayer::Model::Service::TranscodingError error);
    void handleCanceled(const QString &taskId);

private:
    [[nodiscard]] QString nextTaskId() noexcept;
    [[nodiscard]] QString outputPathFor(const QString &inputFilePath,
                                        const QString &outputDirectory,
                                        AudioPlayer::Model::ProcessingOutputFormat format) const;
    [[nodiscard]] QString uniqueOutputPath(const QString &basePath) const;
    [[nodiscard]] int indexOfTask(const QString &taskId) const noexcept;
    [[nodiscard]] int firstPendingTaskIndex() const noexcept;
    void startNextPending();
    void markTaskUpdated(int index);

    ITranscodingBackend *m_backend = nullptr;
    QVector<AudioPlayer::Model::ProcessingTask> m_tasks;
    QSet<QString> m_reservedOutputPaths;
    int m_nextTaskNumber = 1;
};

} // namespace AudioPlayer::Model::Service
