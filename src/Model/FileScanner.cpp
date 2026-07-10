#include "Model/FileScanner.h"

#include <QFileInfo>

namespace AudioPlayer::Model {
namespace {

constexpr auto unknownArtist = "Unknown Artist";
constexpr auto unknownAlbum = "Unknown Album";

QString titleFromFileInfo(const QFileInfo &fileInfo)
{
    QString title = fileInfo.completeBaseName().trimmed();
    if (!title.isEmpty()) {
        return title;
    }

    title = fileInfo.fileName().trimmed();
    if (!title.isEmpty()) {
        return title;
    }

    return fileInfo.filePath().trimmed();
}

} // namespace

QStringList FileScanner::supportedExtensions()
{
    return {"mp3", "wav", "flac"};
}

bool FileScanner::isSupportedAudioFile(const QString &filePath)
{
    const QFileInfo fileInfo(filePath.trimmed());
    const QString extension = fileInfo.suffix().toLower();
    return supportedExtensions().contains(extension);
}

std::optional<AudioFile> FileScanner::scanFile(const QString &filePath)
{
    const QString trimmedPath = filePath.trimmed();
    if (trimmedPath.isEmpty()) {
        return std::nullopt;
    }

    const QFileInfo fileInfo(trimmedPath);
    if (!fileInfo.exists() || !fileInfo.isFile() || !isSupportedAudioFile(trimmedPath)) {
        return std::nullopt;
    }

    return AudioFile(fileInfo.absoluteFilePath(),
                     titleFromFileInfo(fileInfo),
                     QString::fromLatin1(unknownArtist),
                     QString::fromLatin1(unknownAlbum),
                     0);
}

} // namespace AudioPlayer::Model
