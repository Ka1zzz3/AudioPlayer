#pragma once

#include "Common/ViewCommand.h"
#include "ViewModel/SongListModel.h"

#include <QObject>
#include <QString>
#include <QStringList>

namespace AudioPlayer::ViewModel {

class LibraryViewModelProtocol : public QObject
{
    Q_OBJECT

public:
    explicit LibraryViewModelProtocol(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    [[nodiscard]] virtual SongListModel *songs() noexcept = 0;
    [[nodiscard]] virtual const QString &storagePath() const noexcept = 0;
    virtual void setStoragePath(QString storagePath) = 0;
    [[nodiscard]] virtual const QString &scanDirectoryPath() const noexcept = 0;
    virtual void setScanDirectoryPath(QString scanDirectoryPath) = 0;
    [[nodiscard]] virtual int count() const noexcept = 0;
    [[nodiscard]] virtual const QString &lastError() const noexcept = 0;
    [[nodiscard]] virtual const QStringList &warnings() const noexcept = 0;
    [[nodiscard]] virtual const QString &statusMessage() const noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *scanCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *loadCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *saveCommand() noexcept = 0;
    [[nodiscard]] virtual Common::ViewCommand *refreshCommand() noexcept = 0;

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
};

} // namespace AudioPlayer::ViewModel
