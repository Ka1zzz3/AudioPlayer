# Playback Rate + Equalizer Phase Verification

This phase adds session-only playback speed control with pitch preservation and a real 5-band equalizer for the Qt Widgets player.

## Scope delivered

- Fixed playback rates: `0.5x`, `0.75x`, `1x`, `1.25x`, `1.5x`, `2x`.
- Pitch-preserving speed backend path through GStreamer `scaletempo`.
- 5-band equalizer using GStreamer `equalizer-nbands`.
- EQ bands: `60 Hz`, `230 Hz`, `910 Hz`, `3.6 kHz`, `14 kHz`.
- EQ gain range: `-12 dB` to `+12 dB`.
- Presets: `Flat`, `Bass Boost`, `Vocal`, `Treble`.
- Separate `AudioEffectsViewModelProtocol`, `AudioEffectsViewModel`, `AudioEffectsUseCase`, and fakeable `IAudioEffectsService`.
- GStreamer backend hidden behind service boundaries; View/ViewModel protocols do not expose GStreamer, Qt Multimedia, or `QProcess` types.
- Qt Multimedia remains ordinary-playback fallback only when GStreamer effects backend is unavailable.
- Effects settings are session-only and are not written to `storage/library.json` or a settings file.

## Runtime and build dependencies

System package direction for Ubuntu/Debian-like environments:

```bash
sudo apt-get install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
  gstreamer1.0-plugins-base gstreamer1.0-plugins-good
```

Useful diagnostics:

```bash
pkg-config --modversion gstreamer-1.0 gstreamer-audio-1.0 gstreamer-pbutils-1.0
gst-inspect-1.0 scaletempo
gst-inspect-1.0 equalizer-nbands
gst-inspect-1.0 fakesink
```

CMake option:

```bash
-DAUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS=ON
```

When enabled and dependencies are present, the app compiles the GStreamer effects backend and the `AudioPlayerGStreamerEffectsProbe` target. When dependencies are missing, configure prints a clear warning; the Qt Multimedia fallback can still build, but real effects are unavailable.

vcpkg manifest is updated with `gstreamer` using `plugins-base` and `plugins-good`. vcpkg network/package failures remain environment blockers, not source blockers.

## Automated verification evidence

Observed on 2026-07-16 in `build/system-widgets`:

```bash
cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON
cmake --build build/system-widgets --target AudioPlayerGStreamerEffectsProbe
./build/system-widgets/AudioPlayerGStreamerEffectsProbe
cmake --build build/system-widgets --target AudioPlayerGStreamerEffectsBackendTest
ctest --test-dir build/system-widgets -R 'AudioPlayerGStreamerEffectsBackendTest|AudioPlayerAudioEffects.*Test' --output-on-failure
./build/system-widgets/AudioPlayerGStreamerEffectsProbe '/home/kaizzz/projects/music/陳粒 - 小半.flac'
ctest --test-dir build/system-widgets --output-on-failure
cmake --build build/system-widgets --target AudioPlayer
```

Results:

- GStreamer dev/runtime gate passed after installing dev packages.
- Factory probe passed for `scaletempo`, `equalizer-nbands`, and required pipeline elements.
- Fake-sink smoke passed with a local FLAC file: decode → `scaletempo` → `equalizer-nbands` → `fakesink` at `1.25x`.
- Audio effects model/usecase/viewmodel/backend/fallback tests passed.
- Full default test suite passed: 24/24 enabled tests passed; `AudioPlayerQtMultimediaPlaybackServiceTest` remained disabled by default.
- App build produced `build/system-widgets/bin/AudioPlayer`.
- Fallback build also succeeded:

```bash
cmake -S . -B build/system-widgets-gst-off -DAUDIOPLAYER_BUILD_APP=ON -DAUDIOPLAYER_ENABLE_GSTREAMER_EFFECTS=OFF
cmake --build build/system-widgets-gst-off --target AudioPlayer
```

## Required audible manual smoke

Final product acceptance requires a human audible smoke test with an audio output device:

1. Build and launch:

   ```bash
   cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON
   cmake --build build/system-widgets
   ./build/system-widgets/bin/AudioPlayer
   ```

2. Load or scan a vocal track and start playback.
3. Change speed to `0.5x`, `1.5x`, and `2x`.
   - Expected: tempo changes and vocal pitch remains recognizably stable rather than chipmunk/deep pitch shifting.
4. Toggle EQ enabled.
5. Apply `Bass Boost`, `Vocal`, and `Treble` presets.
   - Expected: audible tonal differences while playback continues.
6. Seek, pause/resume, next, and previous.
   - Expected: current session speed/EQ settings remain active.
7. Disable EQ or reset Flat and reset speed to `1x`.

If the app falls back to Qt Multimedia, the effects panel should show unavailable/disabled behavior and this manual smoke does not count as real-effects acceptance.

## Known limitations

- No persistence for speed/EQ settings by design.
- No user presets.
- No spectrum/visualization.
- No plugin system.
- EQ does not affect transcoding output.
- Playback speed does not affect transcode tasks.
- GStreamer package/plugin availability varies by platform and distribution.
- Automated tests avoid real audio devices; audible verification remains manual.
