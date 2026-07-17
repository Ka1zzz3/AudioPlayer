# Qt Widgets Strict MVVM Architecture

The Widgets application follows a strict MVVM split:

- `src/App/`: composition and lifecycle only. `AppCompositionRoot` creates Model services, ViewModels, the `MainWindow`, and `MainWindowPart`. `MainWindowPart` binds ViewModel properties, commands, and events to View ports/signals.
- `src/View/`: UI only. `MainWindow` creates widgets, renders primitive state and `QAbstractItemModel` instances, exposes `setXxxCommand`/state setter ports, emits user-intent signals, and executes injected `ViewCommand` objects. It has zero ViewModel protocol/concrete dependencies.
- `src/ViewModel/`: presentation state and commands. ViewModels own `ViewCommand` members, convert Model data to drawable Qt state, and call Model use cases/services from command handlers or property setters.
- `src/ViewModel/ShellViewModel.*`: presentation-level cross-feature coordination, such as library/playlist projection and playlist playback requests. This keeps ordinary workflow logic out of `AppCompositionRoot`.
- `src/Model/`: domain data, repositories, low-level backends, and Model services/use cases. `ProcessingPlaylistIntegrationService` owns the processing-success-to-transcoded-playlist workflow.
- `src/Common/`: minimal cross-layer primitives such as `ViewCommand`.

## Dependency direction

```text
App/Part -> View + ViewModel + Model composition/binding
View     -> Common::ViewCommand + View signals + Qt display models
ViewModel -> Model/Service/UseCase
Model    -> Model events/signals -> ViewModel or Model integration service
```

Forbidden dependencies are locked by `AudioPlayerArchitectureBoundaryTest`: View must not include/call ViewModel or Model APIs, View-facing protocols must not expose Model/backend types, `PlaybackViewModel` observes `PlaybackUseCase` rather than `IPlaybackService`, and `AppCompositionRoot` must not contain business workflow orchestration.

## Build targets

- `AudioPlayer`: production Qt Widgets application.
- Test targets under `test/`: Model, UseCase, ViewModel, composition, and architecture-boundary checks.

For model/viewmodel-only environments, configure with `-DAUDIOPLAYER_BUILD_APP=OFF`. For environments without GStreamer development/runtime packages, use `-DAUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS=OFF`.
