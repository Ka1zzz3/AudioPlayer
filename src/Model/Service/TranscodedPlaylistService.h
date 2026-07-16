#pragma once

#include "Model/AudioFile.h"
#include "Model/Service/PlaylistCollectionUseCase.h"

#include <QString>

namespace AudioPlayer::Model::Service {

struct TranscodedPlaylistResult
{
    bool addedToPlaylist = false;
    QString status;
    QString error;

    [[nodiscard]] bool ok() const noexcept { return error.isEmpty(); }
};

class TranscodedPlaylistService
{
public:
    explicit TranscodedPlaylistService(QString storagePath,
                                       PlaylistCollectionUseCase useCase = PlaylistCollectionUseCase{});

    [[nodiscard]] const QString &storagePath() const noexcept;
    void setStoragePath(QString storagePath);

    [[nodiscard]] TranscodedPlaylistResult addOutput(const AudioFile &song,
                                                     const QDateTime &timestamp) const;

    static constexpr auto TranscodedPlaylistId = "transcoded";
    static constexpr auto TranscodedPlaylistName = "Transcoded";

private:
    QString m_storagePath;
    PlaylistCollectionUseCase m_useCase;
};

} // namespace AudioPlayer::Model::Service
