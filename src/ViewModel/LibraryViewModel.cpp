#include "ViewModel/LibraryViewModel.h"

#include "Model/FileScanner.h"
#include "Model/JsonSongRepository.h"

#include <optional>
#include <utility>

#include <QStringList>

namespace AudioPlayer::ViewModel {

LibraryViewModel::LibraryViewModel(QObject *parent)
    : QObject(parent)
    , m_songs(this)
{
}

SongListModel *LibraryViewModel::songs() noexcept
{
    return &m_songs;
}

const QString &LibraryViewModel::storagePath() const noexcept
{
    return m_storagePath;
}

void LibraryViewModel::setStoragePath(QString storagePath)
{
    if (m_storagePath == storagePath) {
        return;
    }

    m_storagePath = std::move(storagePath);
    emit storagePathChanged();
}

int LibraryViewModel::count() const noexcept
{
    return m_songs.playList().size();
}

const QString &LibraryViewModel::lastError() const noexcept
{
    return m_lastError;
}

bool LibraryViewModel::load()
{
    QString errorMessage;
    Model::JsonSongRepository repository(m_storagePath);
    std::optional<Model::PlayList> loadedPlayList = repository.load(&errorMessage);
    if (!loadedPlayList.has_value()) {
        setLastError(std::move(errorMessage));
        emit loadFailed(m_lastError);
        return false;
    }

    replacePlayList(std::move(*loadedPlayList));
    setLastError({});
    emit loaded();
    return true;
}

bool LibraryViewModel::refresh()
{
    Model::PlayList refreshedPlayList;
    QStringList skippedFilePaths;
    for (const Model::AudioFile &audioFile : m_songs.playList().items()) {
        std::optional<Model::AudioFile> scannedAudioFile = Model::FileScanner::scanFile(audioFile.filePath());
        if (!scannedAudioFile.has_value()) {
            skippedFilePaths.append(audioFile.filePath());
            continue;
        }

        refreshedPlayList.add(std::move(*scannedAudioFile));
    }

    replacePlayList(std::move(refreshedPlayList));
    if (skippedFilePaths.isEmpty()) {
        setLastError({});
    } else {
        setLastError(QStringLiteral("Refresh skipped %1 missing or unsupported file(s): %2")
                         .arg(skippedFilePaths.size())
                         .arg(skippedFilePaths.join(QStringLiteral(", "))));
    }
    emit refreshed();
    return true;
}

void LibraryViewModel::setLastError(QString errorMessage)
{
    if (m_lastError == errorMessage) {
        return;
    }

    m_lastError = std::move(errorMessage);
    emit lastErrorChanged();
}

void LibraryViewModel::replacePlayList(Model::PlayList playList)
{
    const int previousCount = count();
    m_songs.setPlayList(std::move(playList));
    if (count() != previousCount) {
        emit countChanged();
    }
}

} // namespace AudioPlayer::ViewModel
