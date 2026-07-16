#pragma once

#include "Model/ProcessingTask.h"

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

namespace AudioPlayer::ViewModel {

class ProcessingTaskListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum TaskRole {
        IdRole = Qt::UserRole + 1,
        TypeRole,
        InputPathRole,
        OutputPathRole,
        OutputFormatRole,
        StatusRole,
        ProgressPercentRole,
        ProgressIndeterminateRole,
        ErrorMessageRole,
        TechnicalDetailsRole,
        ResultFilePathRole,
    };
    Q_ENUM(TaskRole)

    explicit ProcessingTaskListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const QVector<Model::ProcessingTask> &tasks() const noexcept;
    void setTasks(QVector<Model::ProcessingTask> tasks);

private:
    QVector<Model::ProcessingTask> m_tasks;
};

} // namespace AudioPlayer::ViewModel
