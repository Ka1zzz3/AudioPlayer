# Playback Core Phase 1 Verification

Date: 2026-07-15

## Scope

Implemented the approved Phase 1 playback core path:

- playback service abstraction and fakeable contract
- queue/currentIndex/next/previous/auto-next/skip rules
- standalone `PlaybackViewModelProtocol` / `PlaybackViewModel`
- explicit App composition between library and playback ViewModels
- Qt Multimedia backend adapter using `QMediaPlayer` + `QAudioOutput`
- minimal Qt Widgets playback controls for play, pause, stop, previous, next, seek, volume, mute, state, current track, and errors

No `library.json` schema change was introduced.

## System Qt validation

The daily development build uses the system Qt directory:

```bash
cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON
cmake --build build/system-widgets
ctest --test-dir build/system-widgets --output-on-failure
```

Result on 2026-07-15:

- configure: passed
- build: passed, `build/system-widgets/bin/AudioPlayer` generated
- default tests: passed, 9/9 executed tests passed
- `AudioPlayerQtMultimediaPlaybackServiceTest`: present but disabled by default

## Qt Multimedia smoke note

`AudioPlayerQtMultimediaPlaybackServiceTest` is intentionally kept as a disabled CTest smoke target. In this environment, enabling it caused Qt Multimedia/platform backend initialization to hang after display backend setup. This matches the approved testing strategy: real Qt Multimedia smoke must not make CI or daily source validation depend on a real display/audio backend.

The production adapter still compiles in the app target and is linked against `Qt6::Multimedia`.

## vcpkg note

`vcpkg.json` was already updated for Qt Widgets + Qt Multimedia manifest correctness in G-PB-001. Full vcpkg source download/build verification remains environment-dependent because the earlier local network/proxy Qt source download failure was not a source blocker.
