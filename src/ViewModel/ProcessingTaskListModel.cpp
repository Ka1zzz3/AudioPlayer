#include "ViewModel/ProcessingTaskListModel.h"

namespace AudioPlayer::ViewModel {

ProcessingTaskListModel::ProcessingTaskListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ProcessingTaskListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_tasks.size();
}

QVariant ProcessingTaskListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tasks.size()) {
        return {};
    }

    const auto &task = m_tasks.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case IdRole:
        return task.id();
    case TypeRole:
        return Model::toString(task.type());
    case InputPathRole:
        return task.inputFilePath();
    case OutputPathRole:
        return task.outputFilePath();
    case OutputFormatRole:
        return Model::toString(task.outputFormat());
    case StatusRole:
        return Model::toString(task.status());
    case ProgressPercentRole:
        return task.progressPercent();
    case ProgressIndeterminateRole:
        return task.progressIndeterminate();
    case ErrorMessageRole:
        return task.errorMessage();
    case TechnicalDetailsRole:
        return task.technicalDetails();
    case ResultFilePathRole:
        return task.resultFilePath();
    default:
        return {};
    }
}

QHash<int, QByteArray> ProcessingTaskListModel::roleNames() const
{
    return {{IdRole, "id"},
            {TypeRole, "type"},
            {InputPathRole, "inputPath"},
            {OutputPathRole, "outputPath"},
            {OutputFormatRole, "outputFormat"},
            {StatusRole, "status"},
            {ProgressPercentRole, "progressPercent"},
            {ProgressIndeterminateRole, "progressIndeterminate"},
            {ErrorMessageRole, "errorMessage"},
            {TechnicalDetailsRole, "technicalDetails"},
            {ResultFilePathRole, "resultFilePath"}};
}

const QVector<Model::ProcessingTask> &ProcessingTaskListModel::tasks() const noexcept
{
    return m_tasks;
}

void ProcessingTaskListModel::setTasks(QVector<Model::ProcessingTask> tasks)
{
    beginResetModel();
    m_tasks = std::move(tasks);
    endResetModel();
}

} // namespace AudioPlayer::ViewModel
