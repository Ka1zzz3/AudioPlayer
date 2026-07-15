# Qt Widgets MVVM Architecture

This project now uses a Qt Widgets front end with the same library-management behavior as the earlier QML prototype. The first-phase goal is structural clarity, not new player features or UI polish.

## Layers

- `src/App/`: composition root. Creates `QApplication`, `LibraryUseCase`, `LibraryViewModel`, and `MainWindow`; owns object lifetime and wiring only.
- `src/View/`: Qt Widgets view. `MainWindow` builds controls and binds them to the ViewModel protocol. It does not create the ViewModel, include Model headers, or call library business methods directly.
- `src/ViewModel/`: view-facing state and protocol. `LibraryViewModel` exposes bindable properties, a `QAbstractItemModel` song model, status/error/warning notifications, and `ViewCommand` command objects. It does not expose public `Q_INVOKABLE` load/save/scan/refresh business methods.
- `src/Model/`: domain data and data processing. `AudioFile`, `PlayList`, `FileScanner`, and `JsonSongRepository` remain the low-level model/storage components.
- `src/Model/Service/`: use-case boundary. `LibraryUseCase` coordinates load, save, scan, and refresh workflows over Model components.
- `src/Common/`: small cross-layer primitives only. `ViewCommand` is the minimal command binding object used by the View and ViewModel.

## Dependency direction

```text
App -> View -> ViewModel protocol -> ViewModel -> Model/Service -> Model
              -> Common command primitives
```

The Model layer does not depend on ViewModel or View. The View layer binds to `LibraryViewModelProtocol`, `QAbstractItemModel`, Qt signals, and `ViewCommand::execute()` rather than concrete business methods such as `scanDirectory`, `load`, `save`, or `refresh`. The protocol intentionally returns Qt's `QAbstractItemModel` instead of the concrete `SongListModel` to avoid leaking ViewModel implementation details into the View boundary.

## Build targets

- `AudioPlayer`: production Qt Widgets application.
- Test targets under `test/`: Model, UseCase, and ViewModel behavior checks.
- `AudioPlayerArchitectureBoundaryTest`: static boundary regression for Widgets-only production wiring and View/ViewModel coupling rules.

For model/viewmodel-only environments, configure with `-DAUDIOPLAYER_BUILD_APP=OFF` to avoid requiring a GUI frontend module.

## Preserved behavior

- Existing scan/load/save/refresh behavior is preserved through `LibraryUseCase` and `LibraryViewModel` tests.
- `library.json` remains schema version `1` with the existing song fields.
- No new playback features or data format changes were introduced in this phase.

## Dependency manifest

`vcpkg.json` now keeps `qtbase` with the `widgets` feature and removes QML-era `qtdeclarative`, Qt Quick shader tooling, and QML language-server overrides. Local vcpkg validation may still require network access to download Qt source archives and platform development packages.
