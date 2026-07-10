#pragma once

#include "Model/AudioFile.h"

#include <optional>

#include <QString>
#include <QStringList>

namespace AudioPlayer::Model {

class FileScanner
{
public:
    [[nodiscard]] static QStringList supportedExtensions();
    [[nodiscard]] static bool isSupportedAudioFile(const QString &filePath);
    [[nodiscard]] static std::optional<AudioFile> scanFile(const QString &filePath);
};

} // namespace AudioPlayer::Model
