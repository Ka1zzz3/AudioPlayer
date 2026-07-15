#pragma once

#include "Model/PlayList.h"

#include <QString>
#include <QStringList>

namespace AudioPlayer::Model::Service {

struct LibraryWorkflowResult
{
    PlayList playList;
    QStringList warnings;
    QString error;

    [[nodiscard]] bool ok() const noexcept;
};

class LibraryUseCase
{
public:
    [[nodiscard]] LibraryWorkflowResult load(const QString &storagePath) const;
    [[nodiscard]] LibraryWorkflowResult save(const QString &storagePath, const PlayList &playList) const;
    [[nodiscard]] LibraryWorkflowResult scanDirectory(const QString &directoryPath) const;
    [[nodiscard]] LibraryWorkflowResult refresh(const PlayList &playList) const;
};

} // namespace AudioPlayer::Model::Service
