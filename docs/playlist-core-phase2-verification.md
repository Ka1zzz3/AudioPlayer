# Playlist Core Phase 2 Verification

Date: 2026-07-16
Scope: Phase 2 playlist-only minimum loop

## Delivered scope

- Created v2 playlist document model: `LibraryDocument`, `LibraryPlaylist`.
- Added v2-only `JsonLibraryDocumentRepository` for `storage/library.json`.
- Added `PlaylistCollectionUseCase` as the v2 document workflow owner.
- Added `PlaylistCollectionViewModelProtocol`, ViewModel, and playlist list model.
- Decoupled visible playlist changes from playback queue changes.
- Added minimal Qt Widgets playlist controls.
- Hardened architecture and playback regression tests.

## Explicitly out of scope

- Playback history.
- v1 compatibility, v1 migration, and v1 golden tests.
- Advanced queue management, shuffle/repeat, search/filter, tag editing.
- UI polish.
- New dependency frameworks, DI container, service locator, event bus, or global state.

## Final validation

Run from repository root with system Qt/system packages:

```bash
cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON
cmake --build build/system-widgets
ctest --test-dir build/system-widgets --output-on-failure
```

Actual result on 2026-07-16:

- `cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON`: passed.
- `cmake --build build/system-widgets`: passed.
- `ctest --test-dir build/system-widgets --output-on-failure`: 13/13 enabled tests passed; `AudioPlayerQtMultimediaPlaybackServiceTest` remained disabled because it is a real Qt Multimedia smoke/integration test and should not require a real audio device in CI.

## Storage contract

The default playlist storage path is `storage/library.json`. The only supported schema for this phase is `schemaVersion: 2`; old v1 flat `songs[]`, unknown schema, missing schema, and corrupt JSON are rejected rather than migrated or overwritten by ordinary save.
