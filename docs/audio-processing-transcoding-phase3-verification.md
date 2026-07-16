# Audio Processing + Transcoding Phase 3 Verification

Phase 3 adds an in-memory audio processing task pipeline and the first real processing capability: batch transcoding.

## Scope delivered

- Select one or more input files from the Qt Widgets UI.
- Select an output directory.
- Choose output format: `mp3`, `wav`, or `flac`.
- Enqueue serial transcode tasks.
- Show task status, output path, progress or indeterminate progress, and visible errors.
- Cancel selected or all pending/running tasks.
- Prompt on application close when processing tasks are pending/running.
- Use FFmpeg CLI through a `QProcess` backend isolated behind `ITranscodingBackend`.
- Add successful outputs to a persistent `Transcoded` playlist through the v2 playlist persistence seam.
- Preserve playback decoupling: transcode completion does not replace the playback queue and does not auto-play.

## Runtime dependency

The application builds without FFmpeg libraries. Real transcoding requires the `ffmpeg` and `ffprobe` executables at runtime.

CMake performs optional `find_program` lookup for both tools. If they are absent, configure/build/test still succeed; transcode tasks report a visible backend/tool error instead of blocking normal app use.

Useful diagnostics:

```bash
which ffmpeg
which ffprobe
ffmpeg -version
ffprobe -version
```

## Non-goals for this phase

- Persistent processing task history or restart recovery.
- Concurrent task execution.
- Advanced transcoding parameters such as bitrate, sample rate, channels, or quality presets.
- Metadata preservation guarantees.
- Automatic overwrite.
- Automatic retry.
- Denoise, vocal separation, equalizer, spectrum, or other processing algorithms.
- QML/Qt Quick UI.
- FFmpeg C API integration.

## Verification evidence

Final commands for the phase:

```bash
cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON
cmake --build build/system-widgets
ctest --test-dir build/system-widgets --output-on-failure
```

Observed on 2026-07-16 in `build/system-widgets`:

- `cmake -S . -B build/system-widgets -DAUDIOPLAYER_BUILD_APP=ON` succeeded.
- Configure reported missing `ffmpeg`/`ffprobe` as non-fatal runtime availability notes.
- `cmake --build build/system-widgets` succeeded and produced `build/system-widgets/bin/AudioPlayer`.
- `ctest --test-dir build/system-widgets --output-on-failure` passed: 19/19 enabled tests passed.
- `AudioPlayerQtMultimediaPlaybackServiceTest` remained disabled by default.

Default enabled tests therefore pass without requiring real ffmpeg, codecs, audio hardware, or GUI automation. The Qt Multimedia playback smoke test remains disabled by default.
