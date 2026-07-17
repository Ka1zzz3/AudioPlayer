# ViewModel

Presentation state and commands live here.

- ViewModels expose Qt properties/signals, `QAbstractItemModel` display models, and owned `Common::ViewCommand` objects.
- Command handlers call Model use cases/services; Views never call ViewModel business methods directly.
- `ShellViewModel` owns cross-ViewModel presentation workflows such as library/playlist projection and playlist playback handoff.
- Default production construction is dependency-injected by App/Part; tests use local helpers/fakes rather than ViewModels creating their own use cases.
- View-facing protocol headers are kept free of Model/backend types. Internal coordination APIs may use Model types, but they are consumed by ViewModel/App binding code, not by `View`.
