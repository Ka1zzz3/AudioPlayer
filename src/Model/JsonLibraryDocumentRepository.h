#pragma once

#include "Model/LibraryDocument.h"

#include <optional>

#include <QString>

namespace AudioPlayer::Model {

class JsonLibraryDocumentRepository
{
public:
    enum class Error {
        None,
        EmptyStoragePath,
        FileNotFound,
        ReadFailed,
        ParseFailed,
        InvalidRoot,
        InvalidSchema,
        InvalidDocument,
        WriteFailed,
        CommitFailed,
    };

    struct LoadResult {
        std::optional<LibraryDocument> document;
        Error error = Error::None;
        QString message;
    };

    struct SaveResult {
        bool saved = false;
        Error error = Error::None;
        QString message;
    };

    explicit JsonLibraryDocumentRepository(QString storagePath = defaultStoragePath());

    [[nodiscard]] const QString &storagePath() const noexcept;
    [[nodiscard]] static QString defaultStoragePath();

    [[nodiscard]] LoadResult load() const;
    [[nodiscard]] SaveResult save(const LibraryDocument &document) const;
    [[nodiscard]] SaveResult replaceWithEmptyV2(const LibraryDocument &document) const;

private:
    [[nodiscard]] SaveResult saveInternal(const LibraryDocument &document, bool allowReplace) const;

    QString m_storagePath;
};

} // namespace AudioPlayer::Model
