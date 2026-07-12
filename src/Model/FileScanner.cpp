#include "Model/FileScanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>

#include <algorithm>
#include <utility>

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

bool ScanResult::ok() const noexcept
{
    return error.isEmpty();
}

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

ScanResult FileScanner::scanDirectory(const QString &directoryPath, bool recursive)
{
    ScanResult result;
    const QString trimmedPath = directoryPath.trimmed();
    if (trimmedPath.isEmpty()) {
        result.error = QStringLiteral("Directory path is empty.");
        return result;
    }

    const QFileInfo directoryInfo(trimmedPath);
    if (!directoryInfo.exists()) {
        result.error = QStringLiteral("Directory does not exist: %1").arg(trimmedPath);
        return result;
    }
    if (!directoryInfo.isDir()) {
        result.error = QStringLiteral("Path is not a directory: %1").arg(directoryInfo.absoluteFilePath());
        return result;
    }
    if (!directoryInfo.isReadable()) {
        result.error = QStringLiteral("Directory is not readable: %1").arg(directoryInfo.absoluteFilePath());
        return result;
    }

    const QDir directory(directoryInfo.absoluteFilePath());
    const QDirIterator::IteratorFlags iteratorFlags = recursive
        ? QDirIterator::Subdirectories
        : QDirIterator::NoIteratorFlags;
    QDirIterator iterator(directory.absolutePath(),
                          QDir::Files | QDir::NoDotAndDotDot | QDir::Readable | QDir::Hidden,
                          iteratorFlags);

    QVector<AudioFile> supportedFiles;
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo fileInfo = iterator.fileInfo();
        const QString absoluteFilePath = fileInfo.absoluteFilePath();
        if (!isSupportedAudioFile(absoluteFilePath)) {
            result.warnings.append(QStringLiteral("Unsupported file skipped: %1").arg(absoluteFilePath));
            continue;
        }

        supportedFiles.append(AudioFile(absoluteFilePath,
                                        titleFromFileInfo(fileInfo),
                                        QString::fromLatin1(unknownArtist),
                                        QString::fromLatin1(unknownAlbum),
                                        0));
    }

    std::sort(supportedFiles.begin(), supportedFiles.end(), [](const AudioFile &left, const AudioFile &right) {
        return QString::compare(left.filePath(), right.filePath(), Qt::CaseSensitive) < 0;
    });

    for (AudioFile &audioFile : supportedFiles) {
        result.playList.add(std::move(audioFile));
    }

    return result;
}

} // namespace AudioPlayer::Model
