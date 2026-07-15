#include "Model/Service/LibraryUseCase.h"

#include "Model/FileScanner.h"
#include "Model/JsonSongRepository.h"

#include <optional>
#include <utility>

namespace AudioPlayer::Model::Service {

bool LibraryWorkflowResult::ok() const noexcept
{
    return error.isEmpty();
}

LibraryWorkflowResult LibraryUseCase::load(const QString &storagePath) const
{
    LibraryWorkflowResult result;
    JsonSongRepository repository(storagePath);
    std::optional<PlayList> loadedPlayList = repository.load(&result.error);
    if (!loadedPlayList.has_value()) {
        return result;
    }

    result.playList = std::move(*loadedPlayList);
    return result;
}

LibraryWorkflowResult LibraryUseCase::save(const QString &storagePath, const PlayList &playList) const
{
    LibraryWorkflowResult result;
    JsonSongRepository repository(storagePath);
    if (!repository.save(playList, &result.error)) {
        return result;
    }

    result.playList = playList;
    return result;
}

LibraryWorkflowResult LibraryUseCase::scanDirectory(const QString &directoryPath) const
{
    const ScanResult scanResult = FileScanner::scanDirectory(directoryPath);

    LibraryWorkflowResult result;
    result.playList = scanResult.playList;
    result.warnings = scanResult.warnings;
    result.error = scanResult.error;
    return result;
}

LibraryWorkflowResult LibraryUseCase::refresh(const PlayList &playList) const
{
    LibraryWorkflowResult result;
    for (const AudioFile &audioFile : playList.items()) {
        std::optional<AudioFile> scannedAudioFile = FileScanner::scanFile(audioFile.filePath());
        if (!scannedAudioFile.has_value()) {
            result.warnings.append(audioFile.filePath());
            continue;
        }

        result.playList.add(std::move(*scannedAudioFile));
    }

    return result;
}

} // namespace AudioPlayer::Model::Service
