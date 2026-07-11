#include "ViewModel/SongListModel.h"

#include "Model/AudioFile.h"

#include <utility>

namespace AudioPlayer::ViewModel {

SongListModel::SongListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SongListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_playList.size();
}

QVariant SongListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_playList.size()) {
        return {};
    }

    const Model::AudioFile *audioFile = m_playList.at(index.row());
    if (audioFile == nullptr) {
        return {};
    }

    switch (role) {
    case FilePathRole:
        return audioFile->filePath();
    case TitleRole:
        return audioFile->title();
    case ArtistRole:
        return audioFile->artist();
    case AlbumRole:
        return audioFile->album();
    case DurationSecondsRole:
        return audioFile->durationSeconds();
    case DisplayTitleRole:
    case Qt::DisplayRole:
        return audioFile->displayTitle();
    default:
        return {};
    }
}

QHash<int, QByteArray> SongListModel::roleNames() const
{
    return {
        {FilePathRole, "filePath"},
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {AlbumRole, "album"},
        {DurationSecondsRole, "durationSeconds"},
        {DisplayTitleRole, "displayTitle"},
    };
}

const Model::PlayList &SongListModel::playList() const noexcept
{
    return m_playList;
}

void SongListModel::setPlayList(Model::PlayList playList)
{
    beginResetModel();
    m_playList = std::move(playList);
    endResetModel();
}

} // namespace AudioPlayer::ViewModel
