#pragma once

#include "ViewModel/ProcessingTaskListModel.h"
#include "ViewModel/ProcessingViewModelProtocol.h"

#include "Model/Service/ProcessingUseCase.h"

#include <memory>

namespace AudioPlayer::ViewModel {

class ProcessingViewModel : public ProcessingViewModelProtocol
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *tasks READ tasks CONSTANT)
    Q_PROPERTY(QStringList inputFilePaths READ inputFilePaths WRITE setInputFilePaths NOTIFY inputFilePathsChanged)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY outputDirectoryChanged)
    Q_PROPERTY(QString outputFormat READ outputFormat WRITE setOutputFormat NOTIFY outputFormatChanged)
    Q_PROPERTY(int selectedTaskRow READ selectedTaskRow WRITE setSelectedTaskRow NOTIFY selectedTaskRowChanged)
    Q_PROPERTY(bool hasPendingOrRunningTasks READ hasPendingOrRunningTasks NOTIFY hasPendingOrRunningTasksChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *enqueueCommand READ enqueueCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *cancelSelectedCommand READ cancelSelectedCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *cancelAllCommand READ cancelAllCommand CONSTANT)

public:
    explicit ProcessingViewModel(std::shared_ptr<Model::Service::ProcessingUseCase> useCase,
                                 QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *tasks() noexcept override;
    [[nodiscard]] const QStringList &inputFilePaths() const noexcept override;
    void setInputFilePaths(QStringList paths) override;
    [[nodiscard]] const QString &outputDirectory() const noexcept override;
    void setOutputDirectory(QString directory) override;
    [[nodiscard]] const QString &outputFormat() const noexcept override;
    void setOutputFormat(QString format) override;
    [[nodiscard]] int selectedTaskRow() const noexcept override;
    void setSelectedTaskRow(int row) override;
    [[nodiscard]] bool hasPendingOrRunningTasks() const noexcept override;
    [[nodiscard]] const QString &lastError() const noexcept override;
    [[nodiscard]] const QString &statusMessage() const noexcept override;

    [[nodiscard]] Common::ViewCommand *enqueueCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *cancelSelectedCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *cancelAllCommand() noexcept override;

    void reportIntegrationWarning(QString warning);

private:
    bool executeEnqueue();
    bool executeCancelSelected();
    bool executeCancelAll();
    void refreshTasks();
    void refreshActiveFlag();
    void setLastError(QString error);
    void setStatusMessage(QString status);
    [[nodiscard]] Model::ProcessingOutputFormat parsedOutputFormat() const noexcept;

    std::shared_ptr<Model::Service::ProcessingUseCase> m_useCase;
    ProcessingTaskListModel m_tasks;
    Common::ViewCommand m_enqueueCommand;
    Common::ViewCommand m_cancelSelectedCommand;
    Common::ViewCommand m_cancelAllCommand;
    QStringList m_inputFilePaths;
    QString m_outputDirectory;
    QString m_outputFormat = QStringLiteral("mp3");
    int m_selectedTaskRow = -1;
    bool m_hasPendingOrRunningTasks = false;
    QString m_lastError;
    QString m_statusMessage;
};

} // namespace AudioPlayer::ViewModel
