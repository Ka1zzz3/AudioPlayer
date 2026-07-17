#pragma once

#include <QObject>
#include <QSet>
#include <QString>

namespace AudioPlayer::Model::Service {

class ProcessingUseCase;
class TranscodedPlaylistService;

class ProcessingPlaylistIntegrationService : public QObject
{
    Q_OBJECT

public:
    ProcessingPlaylistIntegrationService(ProcessingUseCase &processingUseCase,
                                         TranscodedPlaylistService &playlistService,
                                         QObject *parent = nullptr);

public slots:
    void setStoragePath(QString storagePath);

signals:
    void integrationWarning(QString warning);
    void integrationStatus(QString status);

private slots:
    void integrateTask(QString taskId);

private:
    ProcessingUseCase &m_processingUseCase;
    TranscodedPlaylistService &m_playlistService;
    QSet<QString> m_integratedTaskIds;
};

} // namespace AudioPlayer::Model::Service
