#pragma once

#include "Common/ViewCommand.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QStringList>

namespace AudioPlayer::ViewModel {

class ProcessingViewModelProtocol : public QObject
{
    Q_OBJECT

public:
    explicit ProcessingViewModelProtocol(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] virtual QAbstractItemModel *tasks() noexcept = 0;
    [[nodiscard]] virtual const QStringList &inputFilePaths() const noexcept = 0;
    virtual void setInputFilePaths(QStringList paths) = 0;
    [[nodiscard]] virtual const QString &outputDirectory() const noexcept = 0;
    virtual void setOutputDirectory(QString directory) = 0;
    [[nodiscard]] virtual const QString &outputFormat() const noexcept = 0;
    virtual void setOutputFormat(QString format) = 0;
    [[nodiscard]] virtual int selectedTaskRow() const noexcept = 0;
    virtual void setSelectedTaskRow(int row) = 0;
    [[nodiscard]] virtual bool hasPendingOrRunningTasks() const noexcept = 0;
    [[nodiscard]] virtual const QString &lastError() const noexcept = 0;
    [[nodiscard]] virtual const QString &statusMessage() const noexcept = 0;

    [[nodiscard]] virtual Common::ViewCommand *enqueueCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *cancelSelectedCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *cancelAllCommand() noexcept = 0;

signals:
    void inputFilePathsChanged();
    void outputDirectoryChanged();
    void outputFormatChanged();
    void selectedTaskRowChanged();
    void hasPendingOrRunningTasksChanged();
    void lastErrorChanged();
    void statusMessageChanged();
};

} // namespace AudioPlayer::ViewModel
