#include "Model/JsonLibraryDocumentRepository.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace AudioPlayer::Model {
namespace {

using Json = nlohmann::json;

constexpr auto schemaVersionKey = "schemaVersion";
constexpr auto visiblePlaylistIdKey = "visiblePlaylistId";
constexpr auto playlistsKey = "playlists";
constexpr auto idKey = "id";
constexpr auto nameKey = "name";
constexpr auto createdAtKey = "createdAt";
constexpr auto updatedAtKey = "updatedAt";
constexpr auto songsKey = "songs";
constexpr auto filePathKey = "filePath";
constexpr auto titleKey = "title";
constexpr auto artistKey = "artist";
constexpr auto albumKey = "album";
constexpr auto durationSecondsKey = "durationSeconds";

std::string toStdString(const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    return {utf8.constData(), static_cast<std::size_t>(utf8.size())};
}

QString toQString(const std::string &value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

QString errorMessage(JsonLibraryDocumentRepository::Error error, QString detail = {})
{
    switch (error) {
    case JsonLibraryDocumentRepository::Error::None:
        return {};
    case JsonLibraryDocumentRepository::Error::EmptyStoragePath:
        return QStringLiteral("Storage path is empty.");
    case JsonLibraryDocumentRepository::Error::FileNotFound:
        return QStringLiteral("Library document does not exist.");
    case JsonLibraryDocumentRepository::Error::ReadFailed:
        return QStringLiteral("Failed to read library document: %1").arg(detail);
    case JsonLibraryDocumentRepository::Error::ParseFailed:
        return QStringLiteral("Failed to parse library document JSON: %1").arg(detail);
    case JsonLibraryDocumentRepository::Error::InvalidRoot:
        return QStringLiteral("Library document JSON root must be an object.");
    case JsonLibraryDocumentRepository::Error::InvalidSchema:
        return detail.isEmpty() ? QStringLiteral("Library document schema must be version 2.") : detail;
    case JsonLibraryDocumentRepository::Error::InvalidDocument:
        return detail.isEmpty() ? QStringLiteral("Library document is invalid.") : detail;
    case JsonLibraryDocumentRepository::Error::WriteFailed:
        return QStringLiteral("Failed to write library document: %1").arg(detail);
    case JsonLibraryDocumentRepository::Error::CommitFailed:
        return QStringLiteral("Failed to commit library document: %1").arg(detail);
    }

    return QStringLiteral("Unknown library document error.");
}

JsonLibraryDocumentRepository::LoadResult failure(JsonLibraryDocumentRepository::Error error,
                                                  QString detail = {})
{
    return {std::nullopt, error, errorMessage(error, std::move(detail))};
}

JsonLibraryDocumentRepository::SaveResult saveFailure(JsonLibraryDocumentRepository::Error error,
                                                      QString detail = {})
{
    return {false, error, errorMessage(error, std::move(detail))};
}

bool ensureParentDirectory(const QString &storagePath, QString *errorMessage)
{
    const QFileInfo fileInfo(storagePath);
    const QDir parentDirectory = fileInfo.absoluteDir();
    if (parentDirectory.exists()) {
        return true;
    }

    if (QDir().mkpath(parentDirectory.absolutePath())) {
        return true;
    }

    if (errorMessage != nullptr) {
        *errorMessage = QStringLiteral("Failed to create storage directory: %1")
                            .arg(parentDirectory.absolutePath());
    }
    return false;
}

std::optional<Json> parseJsonFile(const QString &storagePath,
                                  JsonLibraryDocumentRepository::Error *error,
                                  QString *message)
{
    QFile file(storagePath);
    if (!file.exists()) {
        *error = JsonLibraryDocumentRepository::Error::FileNotFound;
        *message = errorMessage(*error);
        return std::nullopt;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        *error = JsonLibraryDocumentRepository::Error::ReadFailed;
        *message = errorMessage(*error, file.errorString());
        return std::nullopt;
    }

    try {
        const QByteArray data = file.readAll();
        *error = JsonLibraryDocumentRepository::Error::None;
        message->clear();
        return Json::parse(data.constData(), data.constData() + data.size());
    } catch (const std::exception &exception) {
        *error = JsonLibraryDocumentRepository::Error::ParseFailed;
        *message = errorMessage(*error, QString::fromUtf8(exception.what()));
        return std::nullopt;
    }
}

std::optional<QDateTime> dateTimeFromJson(const Json &object, const char *key, QString *error)
{
    const auto iterator = object.find(key);
    if (iterator == object.end() || !iterator->is_string()) {
        *error = QStringLiteral("Playlist is missing string field: %1").arg(QString::fromLatin1(key));
        return std::nullopt;
    }

    const QDateTime parsed = QDateTime::fromString(toQString(iterator->get<std::string>()), Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        *error = QStringLiteral("Playlist field is not a valid ISO date: %1").arg(QString::fromLatin1(key));
        return std::nullopt;
    }

    return parsed.toUTC();
}

QString requiredString(const Json &object, const char *key, QString *error)
{
    const auto iterator = object.find(key);
    if (iterator == object.end() || !iterator->is_string()) {
        *error = QStringLiteral("JSON object is missing string field: %1").arg(QString::fromLatin1(key));
        return {};
    }

    return toQString(iterator->get<std::string>());
}

Json audioFileToJson(const AudioFile &audioFile)
{
    return Json{
        {filePathKey, toStdString(audioFile.filePath())},
        {titleKey, toStdString(audioFile.title())},
        {artistKey, toStdString(audioFile.artist())},
        {albumKey, toStdString(audioFile.album())},
        {durationSecondsKey, audioFile.durationSeconds()},
    };
}

std::optional<AudioFile> audioFileFromJson(const Json &songJson, QString *error)
{
    if (!songJson.is_object()) {
        *error = QStringLiteral("Song entry must be a JSON object.");
        return std::nullopt;
    }

    const QString filePath = requiredString(songJson, filePathKey, error);
    if (!error->isEmpty()) {
        return std::nullopt;
    }

    qint64 durationSeconds = 0;
    const auto durationIterator = songJson.find(durationSecondsKey);
    if (durationIterator != songJson.end()) {
        if (!durationIterator->is_number_integer()) {
            *error = QStringLiteral("Song duration must be an integer.");
            return std::nullopt;
        }
        durationSeconds = static_cast<qint64>(durationIterator->get<long long>());
    }

    const auto optionalString = [&songJson](const char *key) -> QString {
        const auto iterator = songJson.find(key);
        if (iterator == songJson.end() || !iterator->is_string()) {
            return {};
        }
        return toQString(iterator->get<std::string>());
    };

    AudioFile audioFile(filePath,
                        optionalString(titleKey),
                        optionalString(artistKey),
                        optionalString(albumKey),
                        durationSeconds);
    if (!audioFile.isValid()) {
        *error = QStringLiteral("Song entry has an empty file path.");
        return std::nullopt;
    }

    return audioFile;
}

Json playlistToJson(const LibraryPlaylist &playlist)
{
    Json songs = Json::array();
    for (const AudioFile &song : playlist.songs()) {
        songs.push_back(audioFileToJson(song));
    }

    return Json{
        {idKey, toStdString(playlist.id())},
        {nameKey, toStdString(playlist.name())},
        {createdAtKey, toStdString(playlist.createdAt().toUTC().toString(Qt::ISODateWithMs))},
        {updatedAtKey, toStdString(playlist.updatedAt().toUTC().toString(Qt::ISODateWithMs))},
        {songsKey, std::move(songs)},
    };
}

std::optional<LibraryPlaylist> playlistFromJson(const Json &playlistJson, QString *error)
{
    if (!playlistJson.is_object()) {
        *error = QStringLiteral("Playlist entry must be a JSON object.");
        return std::nullopt;
    }

    const QString id = requiredString(playlistJson, idKey, error);
    if (!error->isEmpty()) {
        return std::nullopt;
    }
    const QString name = requiredString(playlistJson, nameKey, error);
    if (!error->isEmpty()) {
        return std::nullopt;
    }
    std::optional<QDateTime> createdAt = dateTimeFromJson(playlistJson, createdAtKey, error);
    if (!createdAt.has_value()) {
        return std::nullopt;
    }
    std::optional<QDateTime> updatedAt = dateTimeFromJson(playlistJson, updatedAtKey, error);
    if (!updatedAt.has_value()) {
        return std::nullopt;
    }

    LibraryPlaylist playlist(id, name, *createdAt, *updatedAt);
    const auto songsIterator = playlistJson.find(songsKey);
    if (songsIterator == playlistJson.end() || !songsIterator->is_array()) {
        *error = QStringLiteral("Playlist is missing array field: songs");
        return std::nullopt;
    }

    QVector<AudioFile> songs;
    for (const Json &songJson : *songsIterator) {
        std::optional<AudioFile> song = audioFileFromJson(songJson, error);
        if (!song.has_value()) {
            return std::nullopt;
        }
        songs.append(std::move(*song));
    }
    playlist.setSongs(std::move(songs), *updatedAt);
    playlist.setUpdatedAt(*updatedAt);

    if (!playlist.isValid()) {
        *error = QStringLiteral("Playlist has invalid id, name, or timestamps.");
        return std::nullopt;
    }

    return playlist;
}

Json documentToJson(const LibraryDocument &document)
{
    Json playlists = Json::array();
    for (const LibraryPlaylist &playlist : document.playlists()) {
        playlists.push_back(playlistToJson(playlist));
    }

    return Json{
        {schemaVersionKey, LibraryDocument::CurrentSchemaVersion},
        {visiblePlaylistIdKey, toStdString(document.visiblePlaylistId())},
        {playlistsKey, std::move(playlists)},
    };
}

JsonLibraryDocumentRepository::LoadResult documentFromJson(const Json &json)
{
    if (!json.is_object()) {
        return failure(JsonLibraryDocumentRepository::Error::InvalidRoot);
    }

    const auto schemaIterator = json.find(schemaVersionKey);
    if (schemaIterator == json.end() || !schemaIterator->is_number_integer()) {
        return failure(JsonLibraryDocumentRepository::Error::InvalidSchema,
                       QStringLiteral("Library document is missing integer schemaVersion 2."));
    }
    if (schemaIterator->get<int>() != LibraryDocument::CurrentSchemaVersion) {
        return failure(JsonLibraryDocumentRepository::Error::InvalidSchema,
                       QStringLiteral("Library document schemaVersion must be 2."));
    }

    QString error;
    const QString visiblePlaylistId = requiredString(json, visiblePlaylistIdKey, &error);
    if (!error.isEmpty()) {
        return failure(JsonLibraryDocumentRepository::Error::InvalidDocument, error);
    }

    const auto playlistsIterator = json.find(playlistsKey);
    if (playlistsIterator == json.end() || !playlistsIterator->is_array()) {
        return failure(JsonLibraryDocumentRepository::Error::InvalidDocument,
                       QStringLiteral("Library document is missing array field: playlists"));
    }

    QVector<LibraryPlaylist> playlists;
    for (const Json &playlistJson : *playlistsIterator) {
        std::optional<LibraryPlaylist> playlist = playlistFromJson(playlistJson, &error);
        if (!playlist.has_value()) {
            return failure(JsonLibraryDocumentRepository::Error::InvalidDocument, error);
        }
        playlists.append(std::move(*playlist));
    }

    LibraryDocument document(visiblePlaylistId, std::move(playlists));
    if (!document.isValid() || document.visiblePlaylistId() != visiblePlaylistId) {
        return failure(JsonLibraryDocumentRepository::Error::InvalidDocument,
                       QStringLiteral("Library document visiblePlaylistId must reference an existing playlist."));
    }

    return {std::move(document), JsonLibraryDocumentRepository::Error::None, {}};
}

JsonLibraryDocumentRepository::SaveResult validateExistingFileCanBeOverwritten(const QString &storagePath)
{
    QFileInfo fileInfo(storagePath);
    if (!fileInfo.exists()) {
        return {true, JsonLibraryDocumentRepository::Error::None, {}};
    }

    JsonLibraryDocumentRepository::Error parseError = JsonLibraryDocumentRepository::Error::None;
    QString parseMessage;
    std::optional<Json> existingJson = parseJsonFile(storagePath, &parseError, &parseMessage);
    if (!existingJson.has_value()) {
        return {false,
                parseError,
                QStringLiteral("Refusing to overwrite existing non-v2 library document: %1").arg(parseMessage)};
    }

    JsonLibraryDocumentRepository::LoadResult existingDocument = documentFromJson(*existingJson);
    if (!existingDocument.document.has_value()) {
        return {false,
                existingDocument.error,
                QStringLiteral("Refusing to overwrite existing non-v2 library document: %1")
                    .arg(existingDocument.message)};
    }

    return {true, JsonLibraryDocumentRepository::Error::None, {}};
}

} // namespace

JsonLibraryDocumentRepository::JsonLibraryDocumentRepository(QString storagePath)
    : m_storagePath(std::move(storagePath))
{
}

const QString &JsonLibraryDocumentRepository::storagePath() const noexcept
{
    return m_storagePath;
}

QString JsonLibraryDocumentRepository::defaultStoragePath()
{
    return QStringLiteral("storage/library.json");
}

JsonLibraryDocumentRepository::LoadResult JsonLibraryDocumentRepository::load() const
{
    if (m_storagePath.trimmed().isEmpty()) {
        return failure(Error::EmptyStoragePath);
    }

    Error parseError = Error::None;
    QString parseMessage;
    std::optional<Json> json = parseJsonFile(m_storagePath, &parseError, &parseMessage);
    if (!json.has_value()) {
        return {std::nullopt, parseError, parseMessage};
    }

    return documentFromJson(*json);
}

JsonLibraryDocumentRepository::SaveResult JsonLibraryDocumentRepository::save(const LibraryDocument &document) const
{
    return saveInternal(document, false);
}

JsonLibraryDocumentRepository::SaveResult JsonLibraryDocumentRepository::replaceWithEmptyV2(
    const LibraryDocument &document) const
{
    return saveInternal(document, true);
}

JsonLibraryDocumentRepository::SaveResult JsonLibraryDocumentRepository::saveInternal(
    const LibraryDocument &document,
    bool allowReplace) const
{
    if (m_storagePath.trimmed().isEmpty()) {
        return saveFailure(Error::EmptyStoragePath);
    }

    if (!document.isValid()) {
        return saveFailure(Error::InvalidDocument);
    }

    if (!allowReplace) {
        SaveResult preflight = validateExistingFileCanBeOverwritten(m_storagePath);
        if (!preflight.saved) {
            return preflight;
        }
    }

    QString directoryError;
    if (!ensureParentDirectory(m_storagePath, &directoryError)) {
        return saveFailure(Error::WriteFailed, directoryError);
    }

    QSaveFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return saveFailure(Error::WriteFailed, file.errorString());
    }

    const Json json = documentToJson(document);
    const std::string serialized = json.dump(2);
    const qint64 serializedSize = static_cast<qint64>(serialized.size());
    if (file.write(serialized.data(), serializedSize) != serializedSize || file.write("\n", 1) != 1) {
        return saveFailure(Error::WriteFailed, file.errorString());
    }

    if (!file.commit()) {
        return saveFailure(Error::CommitFailed, file.errorString());
    }

    return {true, Error::None, {}};
}

} // namespace AudioPlayer::Model
