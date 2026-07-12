#pragma once

#include "Model/AudioFile.h"
#include "Model/PlayList.h"

#include <optional>

#include <QString>
#include <QStringList>

namespace AudioPlayer::Model {

struct ScanResult
{
    ScanResult() = default;
    ScanResult(const ScanResult &) = default;
    ScanResult(ScanResult &&) = default;
    ScanResult &operator=(const ScanResult &) = default;
    ScanResult &operator=(ScanResult &&) = default;

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
    // Model-level recursive scanning is covered for future service work, but UI
    // callers must keep first-iteration scans non-recursive and synchronous only
    // until an async scan/metadata boundary is introduced.
    [[nodiscard]] static ScanResult scanDirectory(const QString &directoryPath, bool recursive = false);
};

} // namespace AudioPlayer::Model
