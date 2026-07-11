# Model

Domain types and persistence helpers for the local library are implemented here:

- `AudioFile` stores a file path plus title, artist, album, and non-negative duration metadata.
- `PlayList` owns the ordered list of `AudioFile` entries, tracks the current index, and exposes list operations and total duration.
- `FileScanner` recognizes local `.mp3`, `.wav`, and `.flac` files and creates basic `AudioFile` metadata from the file name/path.
- `JsonSongRepository` saves and loads a playlist JSON document for local persistence.

Playback, tag extraction, history, and multi-playlist behavior are outside the current model scope.
