#pragma once

#include "ViewModel/LibraryViewModelProtocol.h"
#include "ViewModel/SongListModel.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <memory>

namespace AudioPlayer::Model::Service {
class LibraryUseCase;
}

namespace AudioPlayer::ViewModel {

namespace ModelService = AudioPlayer::Model::Service;

class LibraryViewModel : public LibraryViewModelProtocol
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *songs READ songs CONSTANT)
    Q_PROPERTY(QString storagePath READ storagePath WRITE setStoragePath NOTIFY storagePathChanged)
    Q_PROPERTY(QString scanDirectoryPath READ scanDirectoryPath WRITE setScanDirectoryPath NOTIFY scanDirectoryPathChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY warningsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    // View-facing intent protocol: Views bind controls to these command
    // objects and consume the state properties/signals above.
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *scanCommand READ scanCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *loadCommand READ loadCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *saveCommand READ saveCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *refreshCommand READ refreshCommand CONSTANT)

public:
    explicit LibraryViewModel(QObject *parent = nullptr);
    explicit LibraryViewModel(std::shared_ptr<const ModelService::LibraryUseCase> libraryUseCase,
                              QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *songs() noexcept override;
    [[nodiscard]] const QString &storagePath() const noexcept override;
    void setStoragePath(QString storagePath) override;
    [[nodiscard]] const QString &scanDirectoryPath() const noexcept override;
    void setScanDirectoryPath(QString scanDirectoryPath) override;
    [[nodiscard]] int count() const noexcept override;
    [[nodiscard]] const QString &lastError() const noexcept override;
    [[nodiscard]] const QStringList &warnings() const noexcept override;
    [[nodiscard]] const QString &statusMessage() const noexcept override;
    [[nodiscard]] Common::ViewCommand *scanCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *loadCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *saveCommand() noexcept override;
    [[nodiscard]] Common::ViewCommand *refreshCommand() noexcept override;

    // C++-internal coordination seam for App composition; Views still consume
    // the library only through LibraryViewModelProtocol and QAbstractItemModel.
    [[nodiscard]] QVector<Model::AudioFile> audioFilesSnapshot() const;

signals:
    void libraryChanged();

private:
    bool executeLoad();
    bool executeRefresh();
    bool executeScanDirectory();
    bool executeScanDirectory(const QString &directoryPath);
    bool executeSave();
    void setLastError(QString errorMessage);
    void setWarnings(QStringList warnings);
    void setStatusMessage(QString statusMessage);
    void replacePlayList(Model::PlayList playList);

    SongListModel m_songs;
    std::shared_ptr<const ModelService::LibraryUseCase> m_libraryUseCase;
    Common::ViewCommand m_scanCommand;
    Common::ViewCommand m_loadCommand;
    Common::ViewCommand m_saveCommand;
    Common::ViewCommand m_refreshCommand;
    QString m_storagePath;
    QString m_scanDirectoryPath;
    QString m_lastError;
    QStringList m_warnings;
    QString m_statusMessage;
};

} // namespace AudioPlayer::ViewModel
