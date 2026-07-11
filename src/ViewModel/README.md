# ViewModel

QML-facing presentation state for the local library is implemented here:

- `SongListModel` adapts `PlayList` to a Qt list model with roles for `filePath`, `title`, `artist`, `album`, `durationSeconds`, and `displayTitle`.
- `LibraryViewModel` owns the song list model, loads playlists from `JsonSongRepository`, refreshes entries through `FileScanner`, exposes `count`, and reports failures through `lastError`/`loadFailed`.

Refresh is deterministic: files that are missing or unsupported are skipped and reported in `lastError`; a refresh with no skipped files clears any stale error.
