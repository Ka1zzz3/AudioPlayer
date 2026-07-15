#include "ViewModel/LibraryViewModel.h"

#include "Model/Service/LibraryUseCase.h"

#include <memory>
#include <utility>

#include <QStringList>

namespace AudioPlayer::ViewModel {

LibraryViewModel::LibraryViewModel(QObject *parent)
    : LibraryViewModel(std::make_shared<ModelService::LibraryUseCase>(), parent)
{
}

LibraryViewModel::LibraryViewModel(std::shared_ptr<const ModelService::LibraryUseCase> libraryUseCase,
                                   QObject *parent)
    : QObject(parent)
    , m_songs(this)
    , m_libraryUseCase(std::move(libraryUseCase))
    , m_scanCommand(QStringLiteral("scan"), [this]() { return scanDirectory(); }, this)
    , m_loadCommand(QStringLiteral("load"), [this]() { return load(); }, this)
    , m_saveCommand(QStringLiteral("save"), [this]() { return save(); }, this)
    , m_refreshCommand(QStringLiteral("refresh"), [this]() { return refresh(); }, this)
{
    if (!m_libraryUseCase) {
        m_libraryUseCase = std::make_shared<ModelService::LibraryUseCase>();
    }
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

Common::ViewCommand *LibraryViewModel::scanCommand() noexcept
{
    return &m_scanCommand;
}

Common::ViewCommand *LibraryViewModel::loadCommand() noexcept
{
    return &m_loadCommand;
}

Common::ViewCommand *LibraryViewModel::saveCommand() noexcept
{
    return &m_saveCommand;
}

Common::ViewCommand *LibraryViewModel::refreshCommand() noexcept
{
    return &m_refreshCommand;
}

bool LibraryViewModel::load()
{
    ModelService::LibraryWorkflowResult result = m_libraryUseCase->load(m_storagePath);
    if (!result.ok()) {
        setLastError(std::move(result.error));
        emit loadFailed(m_lastError);
        return false;
    }

    replacePlayList(std::move(result.playList));
    setLastError({});
    setWarnings({});
    setStatusMessage(QStringLiteral("Loaded %1 song(s).").arg(count()));
    emit loaded();
    return true;
}

bool LibraryViewModel::refresh()
{
    ModelService::LibraryWorkflowResult result = m_libraryUseCase->refresh(m_songs.playList());
    const int skippedCount = result.warnings.size();
    const bool hasWarnings = !result.warnings.isEmpty();
    replacePlayList(std::move(result.playList));
    setLastError({});
    setWarnings(std::move(result.warnings));
    setStatusMessage(!hasWarnings
                         ? QStringLiteral("Refreshed %1 song(s).").arg(count())
                         : QStringLiteral("Refresh skipped %1 missing or unsupported file(s).").arg(skippedCount));
    emit refreshed();
    return true;
}

bool LibraryViewModel::scanDirectory()
{
    return scanDirectory(m_scanDirectoryPath);
}

bool LibraryViewModel::scanDirectory(const QString &directoryPath)
{
    ModelService::LibraryWorkflowResult result = m_libraryUseCase->scanDirectory(directoryPath);
    if (!result.ok()) {
        setLastError(std::move(result.error));
        setWarnings({});
        setStatusMessage(QStringLiteral("Scan failed."));
        return false;
    }

    const int skippedCount = result.warnings.size();
    const bool hasWarnings = !result.warnings.isEmpty();
    replacePlayList(std::move(result.playList));
    setLastError({});
    setWarnings(std::move(result.warnings));
    setStatusMessage(!hasWarnings
                         ? QStringLiteral("Scanned %1 song(s).").arg(count())
                         : QStringLiteral("Scanned %1 song(s), skipped %2 unsupported file(s).")
                               .arg(count())
                               .arg(skippedCount));
    return true;
}

bool LibraryViewModel::save()
{
    ModelService::LibraryWorkflowResult result = m_libraryUseCase->save(m_storagePath, m_songs.playList());
    if (!result.ok()) {
        setLastError(std::move(result.error));
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
