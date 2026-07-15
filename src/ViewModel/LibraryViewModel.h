#pragma once

#include "Common/ViewCommand.h"
#include "ViewModel/SongListModel.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <memory>
#if __has_include(<QtQmlIntegration/qqmlintegration.h>)
#include <QtQmlIntegration/qqmlintegration.h>
#else
#define QML_NAMED_ELEMENT(NAME)
#define QML_ANONYMOUS
#endif

namespace AudioPlayer::Model::Service {
class LibraryUseCase;
}

namespace AudioPlayer::ViewModel {

namespace Common = AudioPlayer::Common;
namespace ModelService = AudioPlayer::Model::Service;

class LibraryViewModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LibraryViewModel)
    Q_PROPERTY(AudioPlayer::ViewModel::SongListModel *songs READ songs CONSTANT)
    Q_PROPERTY(QString storagePath READ storagePath WRITE setStoragePath NOTIFY storagePathChanged)
    Q_PROPERTY(QString scanDirectoryPath READ scanDirectoryPath WRITE setScanDirectoryPath NOTIFY scanDirectoryPathChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QStringList warnings READ warnings NOTIFY warningsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    // View-facing intent protocol: Views should bind controls to these command
    // objects and consume the state properties/signals above. The direct
    // Q_INVOKABLE methods remain temporarily for existing QML/tests.
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *scanCommand READ scanCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *loadCommand READ loadCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *saveCommand READ saveCommand CONSTANT)
    Q_PROPERTY(AudioPlayer::Common::ViewCommand *refreshCommand READ refreshCommand CONSTANT)

public:
    explicit LibraryViewModel(QObject *parent = nullptr);
    explicit LibraryViewModel(std::shared_ptr<const ModelService::LibraryUseCase> libraryUseCase,
                              QObject *parent = nullptr);

    [[nodiscard]] SongListModel *songs() noexcept;
    [[nodiscard]] const QString &storagePath() const noexcept;
    void setStoragePath(QString storagePath);
    [[nodiscard]] const QString &scanDirectoryPath() const noexcept;
    void setScanDirectoryPath(QString scanDirectoryPath);
    [[nodiscard]] int count() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;
    [[nodiscard]] const QStringList &warnings() const noexcept;
    [[nodiscard]] const QString &statusMessage() const noexcept;
    [[nodiscard]] Common::ViewCommand *scanCommand() noexcept;
    [[nodiscard]] Common::ViewCommand *loadCommand() noexcept;
    [[nodiscard]] Common::ViewCommand *saveCommand() noexcept;
    [[nodiscard]] Common::ViewCommand *refreshCommand() noexcept;

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool refresh();
    Q_INVOKABLE bool scanDirectory();
    Q_INVOKABLE bool scanDirectory(const QString &directoryPath);
    Q_INVOKABLE bool save();

signals:
    void loaded();
    void refreshed();
    void loadFailed(const QString &errorMessage);
    void countChanged();
    void storagePathChanged();
    void scanDirectoryPathChanged();
    void lastErrorChanged();
    void warningsChanged();
    void statusMessageChanged();

private:
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
