# ViewModel

Qt Widgets-facing presentation state for the local library is implemented here:

- `LibraryViewModelProtocol` is the stable View boundary. Views bind to its properties, Qt signals, `QAbstractItemModel`, and `ViewCommand` objects instead of concrete ViewModel methods.
- `LibraryViewModel` owns the song list model, delegates load/save/scan/refresh workflows to `Model::Service::LibraryUseCase`, exposes `count`, status, errors, and warnings, and keeps command handlers private.
- `SongListModel` adapts `PlayList` to a Qt item model with roles for `filePath`, `title`, `artist`, `album`, `durationSeconds`, and `displayTitle`.

Refresh is deterministic: files that are missing or unsupported are skipped and reported through `warnings`; a refresh with no skipped files clears any stale error.
