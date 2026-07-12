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

const QString &LibraryViewModel::scanDirectoryPath() const noexcept
{
    return m_scanDirectoryPath;
}

void LibraryViewModel::setScanDirectoryPath(QString scanDirectoryPath)
{
    if (m_scanDirectoryPath == scanDirectoryPath) {
        return;
    }

    m_scanDirectoryPath = std::move(scanDirectoryPath);
    emit scanDirectoryPathChanged();
}

int LibraryViewModel::count() const noexcept
{
    return m_songs.playList().size();
}

const QString &LibraryViewModel::lastError() const noexcept
{
    return m_lastError;
}

const QStringList &LibraryViewModel::warnings() const noexcept
{
    return m_warnings;
}

const QString &LibraryViewModel::statusMessage() const noexcept
{
    return m_statusMessage;
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
    setWarnings({});
    setStatusMessage(QStringLiteral("Loaded %1 song(s).").arg(count()));
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
    setLastError({});
    setWarnings(skippedFilePaths);
    setStatusMessage(skippedFilePaths.isEmpty()
                         ? QStringLiteral("Refreshed %1 song(s).").arg(count())
                         : QStringLiteral("Refresh skipped %1 missing or unsupported file(s).").arg(skippedFilePaths.size()));
    emit refreshed();
    return true;
}

bool LibraryViewModel::scanDirectory()
{
    return scanDirectory(m_scanDirectoryPath);
}

bool LibraryViewModel::scanDirectory(const QString &directoryPath)
{
    const Model::ScanResult result = Model::FileScanner::scanDirectory(directoryPath);
    if (!result.ok()) {
        setLastError(result.error);
        setWarnings({});
        setStatusMessage(QStringLiteral("Scan failed."));
        return false;
    }

    replacePlayList(std::move(Model::PlayList(result.playList)));
    setLastError({});
    setWarnings(result.warnings);
    setStatusMessage(result.warnings.isEmpty()
                         ? QStringLiteral("Scanned %1 song(s).").arg(count())
                         : QStringLiteral("Scanned %1 song(s), skipped %2 unsupported file(s).")
                               .arg(count())
                               .arg(result.warnings.size()));
    return true;
}

bool LibraryViewModel::save()
{
    QString errorMessage;
    Model::JsonSongRepository repository(m_storagePath);
    if (!repository.save(m_songs.playList(), &errorMessage)) {
        setLastError(std::move(errorMessage));
        setStatusMessage(QStringLiteral("Save failed."));
        return false;
    }

    setLastError({});
    setStatusMessage(QStringLiteral("Saved %1 song(s).").arg(count()));
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

void LibraryViewModel::setWarnings(QStringList warnings)
{
    if (m_warnings == warnings) {
        return;
    }

    m_warnings = std::move(warnings);
    emit warningsChanged();
}

void LibraryViewModel::setStatusMessage(QString statusMessage)
{
    if (m_statusMessage == statusMessage) {
        return;
    }

    m_statusMessage = std::move(statusMessage);
    emit statusMessageChanged();
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
