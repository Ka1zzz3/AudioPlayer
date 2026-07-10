#pragma once

#include "Model/PlayList.h"

#include <optional>

#include <QString>

namespace AudioPlayer::Model {

class JsonSongRepository
{
public:
    explicit JsonSongRepository(QString storagePath);

    [[nodiscard]] const QString &storagePath() const noexcept;

    bool save(const PlayList &playList, QString *errorMessage = nullptr) const;
    [[nodiscard]] std::optional<PlayList> load(QString *errorMessage = nullptr) const;

private:
    QString m_storagePath;
};

} // namespace AudioPlayer::Model
