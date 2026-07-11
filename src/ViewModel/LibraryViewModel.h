#pragma once

#include "ViewModel/SongListModel.h"

#include <QObject>
#include <QString>
#if __has_include(<QtQmlIntegration/qqmlintegration.h>)
#include <QtQmlIntegration/qqmlintegration.h>
#else
#define QML_NAMED_ELEMENT(NAME)
#define QML_ANONYMOUS
#endif

namespace AudioPlayer::ViewModel {

class LibraryViewModel : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(LibraryViewModel)
    Q_PROPERTY(AudioPlayer::ViewModel::SongListModel *songs READ songs CONSTANT)
    Q_PROPERTY(QString storagePath READ storagePath WRITE setStoragePath NOTIFY storagePathChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)

public:
    explicit LibraryViewModel(QObject *parent = nullptr);

    [[nodiscard]] SongListModel *songs() noexcept;
    [[nodiscard]] const QString &storagePath() const noexcept;
    void setStoragePath(QString storagePath);
    [[nodiscard]] int count() const noexcept;
    [[nodiscard]] const QString &lastError() const noexcept;

    Q_INVOKABLE bool load();
    Q_INVOKABLE bool refresh();

signals:
    void loaded();
    void refreshed();
    void loadFailed(const QString &errorMessage);
    void countChanged();
    void storagePathChanged();
    void lastErrorChanged();

private:
    void setLastError(QString errorMessage);
    void replacePlayList(Model::PlayList playList);

    SongListModel m_songs;
    QString m_storagePath;
    QString m_lastError;
};

} // namespace AudioPlayer::ViewModel
