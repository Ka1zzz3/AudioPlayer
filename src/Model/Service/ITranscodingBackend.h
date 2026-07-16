#pragma once

#include "Model/ProcessingTypes.h"

#include <QMetaType>
#include <QObject>
#include <QString>

namespace AudioPlayer::Model::Service {

struct TranscodingRequest
{
    QString taskId;
    QString inputFilePath;
    QString outputFilePath;
    AudioPlayer::Model::ProcessingOutputFormat outputFormat = AudioPlayer::Model::ProcessingOutputFormat::Mp3;
};

struct TranscodingProgress
{
    QString taskId;
    int percent = 0;
    bool indeterminate = false;
};

struct TranscodingResult
{
    QString taskId;
    QString outputFilePath;
};

struct TranscodingError
{
    QString taskId;
    QString message;
    QString technicalDetails;
};

class ITranscodingBackend : public QObject
{
    Q_OBJECT

public:
    explicit ITranscodingBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~ITranscodingBackend() override = default;

    [[nodiscard]] virtual bool available() const noexcept = 0;
    [[nodiscard]] virtual QString unavailableReason() const = 0;
    [[nodiscard]] virtual bool busy() const noexcept = 0;
    [[nodiscard]] virtual QString currentTaskId() const = 0;

public slots:
    virtual void startTranscode(AudioPlayer::Model::Service::TranscodingRequest request) = 0;
    virtual void cancelCurrent() = 0;

signals:
    void progressChanged(AudioPlayer::Model::Service::TranscodingProgress progress);
    void transcodeSucceeded(AudioPlayer::Model::Service::TranscodingResult result);
    void transcodeFailed(AudioPlayer::Model::Service::TranscodingError error);
    void transcodeCanceled(const QString &taskId);
};

} // namespace AudioPlayer::Model::Service

Q_DECLARE_METATYPE(AudioPlayer::Model::Service::TranscodingRequest)
Q_DECLARE_METATYPE(AudioPlayer::Model::Service::TranscodingProgress)
Q_DECLARE_METATYPE(AudioPlayer::Model::Service::TranscodingResult)
Q_DECLARE_METATYPE(AudioPlayer::Model::Service::TranscodingError)
