# Storage

The app persists the local library as JSON through `JsonSongRepository`. The current document shape is:

```json
{
  "schemaVersion": 1,
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
```

Parent directories are created on save. Loading validates the JSON root, the `songs` array, required string `filePath` fields, and integer durations.
