#include "Model/Service/ProcessingPlaylistIntegrationService.h"

#include "Model/AudioFile.h"
#include "Model/ProcessingTask.h"
#include "Model/Service/ProcessingUseCase.h"
#include "Model/Service/TranscodedPlaylistService.h"

#include <QDateTime>
#include <QFileInfo>

#include <utility>

namespace AudioPlayer::Model::Service {

ProcessingPlaylistIntegrationService::ProcessingPlaylistIntegrationService(ProcessingUseCase &processingUseCase,
                                                                           TranscodedPlaylistService &playlistService,
                                                                           QObject *parent)
    : QObject(parent)
    , m_processingUseCase(processingUseCase)
    , m_playlistService(playlistService)
{
    connect(&m_processingUseCase,
            &ProcessingUseCase::taskUpdated,
            this,
            &ProcessingPlaylistIntegrationService::integrateTask);
}

void ProcessingPlaylistIntegrationService::setStoragePath(QString storagePath)
{
    m_playlistService.setStoragePath(std::move(storagePath));
}

void ProcessingPlaylistIntegrationService::integrateTask(QString taskId)
{
    if (m_integratedTaskIds.contains(taskId)) {
        return;
    }

    const auto tasks = m_processingUseCase.tasks();
    for (const auto &task : tasks) {
        if (task.id() != taskId || task.status() != ProcessingTaskStatus::Succeeded) {
            continue;
        }

        m_integratedTaskIds.insert(taskId);
        const QFileInfo outputInfo(task.resultFilePath());
        const QString title = outputInfo.completeBaseName().isEmpty()
            ? outputInfo.fileName()
            : outputInfo.completeBaseName();
        const auto result = m_playlistService.addOutput(AudioFile(task.resultFilePath(), title),
                                                        QDateTime::currentDateTimeUtc());
        if (!result.ok()) {
            emit integrationWarning(QStringLiteral("Transcode succeeded, but adding to Transcoded playlist failed: %1")
                                        .arg(result.error));
        } else {
            emit integrationWarning({});
            if (!result.status.isEmpty()) {
                emit integrationStatus(result.status);
            }
        }
        return;
    }
}

} // namespace AudioPlayer::Model::Service
