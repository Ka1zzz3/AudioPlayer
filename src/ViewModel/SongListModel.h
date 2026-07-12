#pragma once

#include "Model/PlayList.h"

#include <QAbstractListModel>
#include <QHash>
#include <QModelIndex>
#include <QByteArray>
#include <QVariant>
#if __has_include(<QtQmlIntegration/qqmlintegration.h>)
#include <QtQmlIntegration/qqmlintegration.h>
#else
#define QML_NAMED_ELEMENT(NAME)
#define QML_ANONYMOUS
#endif

namespace AudioPlayer::ViewModel {

class SongListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    enum SongRole {
        FilePathRole = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationSecondsRole,
        DisplayTitleRole,
        ExtensionRole,
    };
    Q_ENUM(SongRole)

    explicit SongListModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] const Model::PlayList &playList() const noexcept;
    void setPlayList(Model::PlayList playList);

private:
    Model::PlayList m_playList;
};

} // namespace AudioPlayer::ViewModel
