#include "Model/JsonSongRepository.h"

#include <nlohmann/json.hpp>

#include <exception>
#include <utility>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace AudioPlayer::Model {
namespace {

constexpr auto schemaVersionKey = "schemaVersion";
constexpr auto songsKey = "songs";
constexpr auto filePathKey = "filePath";
constexpr auto titleKey = "title";
constexpr auto artistKey = "artist";
constexpr auto albumKey = "album";
constexpr auto durationSecondsKey = "durationSeconds";

using Json = nlohmann::json;

std::string toStdString(const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    return {utf8.constData(), static_cast<std::size_t>(utf8.size())};
}

QString toQString(const std::string &value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

void setError(QString *errorMessage, const QString &message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
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

    setError(errorMessage, QStringLiteral("Failed to create storage directory: %1")
                               .arg(parentDirectory.absolutePath()));
    return false;
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

std::optional<AudioFile> audioFileFromJson(const Json &songJson, QString *errorMessage)
{
    if (!songJson.is_object()) {
        setError(errorMessage, QStringLiteral("Song entry must be a JSON object."));
        return std::nullopt;
    }

    const auto filePathIterator = songJson.find(filePathKey);
    if (filePathIterator == songJson.end() || !filePathIterator->is_string()) {
        setError(errorMessage, QStringLiteral("Song entry is missing string field: %1")
                                   .arg(QString::fromLatin1(filePathKey)));
        return std::nullopt;
    }

    qint64 durationSeconds = 0;
    const auto durationIterator = songJson.find(durationSecondsKey);
    if (durationIterator != songJson.end()) {
        if (!durationIterator->is_number_integer()) {
            setError(errorMessage, QStringLiteral("Song duration must be an integer."));
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

    AudioFile audioFile(toQString(filePathIterator->get<std::string>()),
                        optionalString(titleKey),
                        optionalString(artistKey),
                        optionalString(albumKey),
                        durationSeconds);
    if (!audioFile.isValid()) {
        setError(errorMessage, QStringLiteral("Song entry has an empty file path."));
        return std::nullopt;
    }

    return audioFile;
}

} // namespace

JsonSongRepository::JsonSongRepository(QString storagePath)
    : m_storagePath(std::move(storagePath))
{
}

const QString &JsonSongRepository::storagePath() const noexcept
{
    return m_storagePath;
}

bool JsonSongRepository::save(const PlayList &playList, QString *errorMessage) const
{
    if (m_storagePath.trimmed().isEmpty()) {
        setError(errorMessage, QStringLiteral("Storage path is empty."));
        return false;
    }

    if (!ensureParentDirectory(m_storagePath, errorMessage)) {
        return false;
    }

    Json songs = Json::array();
    for (const AudioFile &audioFile : playList.items()) {
        if (!audioFile.isValid()) {
            setError(errorMessage, QStringLiteral("Cannot save song entry with an empty file path."));
            return false;
        }
        songs.push_back(audioFileToJson(audioFile));
    }

    const Json document{
        {schemaVersionKey, 1},
        {songsKey, std::move(songs)},
    };

    QSaveFile file(m_storagePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setError(errorMessage, QStringLiteral("Failed to open storage file for writing: %1")
                                   .arg(file.errorString()));
        return false;
    }

    const std::string serialized = document.dump(2);
    const qint64 serializedSize = static_cast<qint64>(serialized.size());
    if (file.write(serialized.data(), serializedSize) != serializedSize || file.write("\n", 1) != 1) {
        setError(errorMessage, QStringLiteral("Failed to write storage file: %1")
                                   .arg(file.errorString()));
        return false;
    }

    if (!file.commit()) {
        setError(errorMessage, QStringLiteral("Failed to commit storage file: %1")
                                   .arg(file.errorString()));
        return false;
    }

    setError(errorMessage, {});
    return true;
}

std::optional<PlayList> JsonSongRepository::load(QString *errorMessage) const
{
    if (m_storagePath.trimmed().isEmpty()) {
        setError(errorMessage, QStringLiteral("Storage path is empty."));
        return std::nullopt;
    }

    QFile file(m_storagePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(errorMessage, QStringLiteral("Failed to open storage file for reading: %1")
                                   .arg(file.errorString()));
        return std::nullopt;
    }

    Json document;
    try {
        const QByteArray data = file.readAll();
        document = Json::parse(data.constData(), data.constData() + data.size());
    } catch (const std::exception &exception) {
        setError(errorMessage, QStringLiteral("Failed to parse storage JSON: %1")
                                   .arg(QString::fromUtf8(exception.what())));
        return std::nullopt;
    }

    if (!document.is_object()) {
        setError(errorMessage, QStringLiteral("Storage JSON root must be an object."));
        return std::nullopt;
    }

    const auto songsIterator = document.find(songsKey);
    if (songsIterator == document.end() || !songsIterator->is_array()) {
        setError(errorMessage, QStringLiteral("Storage JSON is missing array field: %1")
                                   .arg(QString::fromLatin1(songsKey)));
        return std::nullopt;
    }

    PlayList playList;
    for (const Json &songJson : *songsIterator) {
        std::optional<AudioFile> audioFile = audioFileFromJson(songJson, errorMessage);
        if (!audioFile.has_value()) {
            return std::nullopt;
        }
        playList.add(std::move(*audioFile));
    }

    setError(errorMessage, {});
    return playList;
}

} // namespace AudioPlayer::Model
