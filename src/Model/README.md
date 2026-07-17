# Model

Domain data, repositories, backend abstractions, and use-case services live here.

- Domain types such as `AudioFile`, `PlayList`, `LibraryDocument`, `LibraryPlaylist`, and `ProcessingTask` contain data and domain behavior only.
- Repository/backend classes handle persistence, scanning, playback backend access, audio effects backend access, and transcoding details.
- Use cases/services expose Model-level operations and events to ViewModels.
- `ProcessingPlaylistIntegrationService` owns the workflow that integrates successful processing outputs into the transcoded playlist, keeping that business workflow out of `AppCompositionRoot` and `View`.
- Model code must not depend on `View` or `ViewModel`.
