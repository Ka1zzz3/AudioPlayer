#pragma once

#include "Model/AudioFile.h"
#include "Model/PlayList.h"

#include <optional>

#include <QString>
#include <QStringList>

namespace AudioPlayer::Model {

struct ScanResult
{
    PlayList playList;
    QStringList warnings;
    QString error;

    [[nodiscard]] bool ok() const noexcept;
};

class FileScanner
{
public:
    [[nodiscard]] static QStringList supportedExtensions();
    [[nodiscard]] static bool isSupportedAudioFile(const QString &filePath);
    [[nodiscard]] static std::optional<AudioFile> scanFile(const QString &filePath);
    [[nodiscard]] static ScanResult scanDirectory(const QString &directoryPath, bool recursive = false);
};

} // namespace AudioPlayer::Model
