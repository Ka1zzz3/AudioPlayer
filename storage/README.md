# Storage

The production playlist document is stored at:

```text
storage/library.json
```

Phase 2 uses a playlist-only JSON format with `schemaVersion: 2` as the only supported target format. The app does not perform v1 compatibility reads or v1 migration in this phase.

## `library.json` v2 shape

```json
{
  "schemaVersion": 2,
  "visiblePlaylistId": "default",
  "playlists": [
    {
      "id": "default",
      "name": "Default",
      "createdAt": "2026-07-15T12:00:00.000Z",
      "updatedAt": "2026-07-15T12:00:00.000Z",
      "songs": [
        {
          "filePath": "/absolute/path/song.mp3",
          "title": "Song",
          "artist": "Artist",
          "album": "Album",
          "durationSeconds": 0
        }
      ]
    }
  ]
}
```

## Persistence rules

- `schemaVersion` must be exactly `2`.
- `visiblePlaylistId` must reference an existing playlist.
- A document must always contain at least one playlist.
- Deleting the last playlist creates an empty `Default` playlist.
- Ordinary saves preflight the existing target file and refuse to overwrite non-v2, unknown-schema, missing-schema, or corrupt JSON files.
- First run with a missing `storage/library.json` may create an empty v2 document.
- Existing v1 flat `songs[]` files are rejected, not migrated.

## Scope notes

Phase 2 is playlist-only:

- No playback history.
- No v1 compatibility or migration.
- No advanced queue management, shuffle/repeat, search/filter, tag editing, import/export, cloud sync, audio processing, EQ, or spectrum features.

Visible playlist switching is independent from the active playback queue. The active playback queue is replaced only through an explicit play request from the playlist ViewModel routed by App Composition.


## Phase 3 transcoded outputs

Successful transcode tasks add their output file to a dedicated playlist:

```text
id: transcoded
name: Transcoded
```

The `Transcoded` playlist is created on demand and saved through the ordinary `schemaVersion: 2` persistence path. If the audio file is created but playlist insertion or save fails, the processing UI reports a visible warning/error; the failure does not mark the file transcode itself as failed.

Adding a transcoded output to `storage/library.json` does not replace the active playback queue and does not auto-play. Playback queue changes still require an explicit play request.
