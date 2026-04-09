# MXTune ARA Implementation Plan (JUCE 8 Edition)

> Last updated after code audit of `src/mx_tune.cpp`, `src/ring_buffer.h`,
> `src/pitch_detector.h`, and `JUCE/Source/PluginProcessor.cpp`.

---

## 1. Purpose & Core Goals

Migrate MXTune from its current real-time `processBlock` scanning model to a
**JUCE 8 ARA (Audio Random Access)** architecture, enabling full-clip analysis
independent of playback.

- **Full-Clip Analysis:** Drive `mx_tune`'s pitch detector from an
  `ARAAudioSourceReader` instead of waiting for the host to play audio through
  `processBlock`.
- **Non-Destructive Editing:** Expose pitch corrections through the ARA Document
  model so the host (Logic Pro, Studio One, Reaper) owns the clip state.
- **Modernized UI:** Leverage JUCE 8's Metal (macOS) and Direct2D (Windows)
  backends for the pitch grid and waveform overlay.

---

## 2. What the Code Audit Revealed

Understanding these facts is required before any implementation work begins.

### 2.1 `ring_buffer` is NOT the integration point

`ring_buffer` (`src/ring_buffer.h`) is a fixed-size circular buffer used
**internally by the pitch detector implementations** as their sample window.
It is not a queue between `processBlock` and the analysis engine.

The actual integration point is `mx_tune::run()`:

```cpp
// src/mx_tune.cpp:326
void mx_tune::run(float* in, float* out, std::int32_t n, float timestamp)
```

This function already takes a flat `float*` buffer and a timestamp. It has no
knowledge of where those samples came from. This is the function
`MXTunePlaybackRegionRenderer` will call, feeding it samples from the ARA
reader instead of from `processBlock`.

### 2.2 `mx_tune::run()` does detection AND shifting in one pass

```cpp
// src/mx_tune.cpp:369 — always runs regardless of _track
out[i] = _delay.process(_shifter->shifter(in[i]));
```

The renderer only needs the detection half (the `_m_tune.set_inpitch()` calls
inside the `if (_track)` block at line 341). Running the full `run()` on the
background thread would:
- Waste CPU on pitch shifting samples that are immediately discarded.
- Write shifted audio into the output buffer unnecessarily.

**Required change:** Add a `scan_only` path to `mx_tune` that runs the
detector and records `inpitch` but skips the shifter entirely. See §3.2.

### 2.3 Thread safety: `_m_tune` cannot be shared

`_m_tune` (`manual_tune`) is written by `run()` via `set_inpitch()` and read
by `processBlock` via `get_outpitch()`. Running the renderer on a background
thread while `processBlock` runs on the audio thread would be a data race.

**Required approach:** The renderer owns a **separate `mx_tune` instance**
configured for scanning only. When the scan completes, results are merged into
the processor's live `_m_tune` via `juce::AsyncUpdater`. See §3.3.

### 2.4 Timestamp source changes for the renderer

In `PluginProcessor.cpp`, `_cur_time` comes from:

```cpp
// PluginProcessor.cpp:230
if (auto t = pos->getTimeInSeconds()) _cur_time = *t;
```

The renderer cannot use the playhead. Instead, it computes timestamps directly
from the clip's ARA position and the reader's sample offset:

```
clip_time = ara_playback_region.startInAudioModificationTime
          + (sample_offset / ara_audio_source.getSampleRate())
```

### 2.5 Existing class name

The processor class in `JUCE/Source/PluginProcessor.cpp` is
`AutotalentAudioProcessor`, not `MXTuneAudioProcessor`. All class hierarchy
notes below use the actual names.

---

## 3. Required Changes to Existing Code

These are surgical changes to `src/` that must be made **before** the ARA
renderer can be written.

### 3.1 No changes to `ring_buffer`

`ring_buffer` is internal detector state and does not need to be touched.

### 3.2 Add `scan()` to `mx_tune`

Add a new public method alongside `run()` that runs only the detection and
`_m_tune.set_inpitch()` path:

```cpp
// Proposed addition to src/mx_tune.h
void scan(const float* in, std::int32_t n, float timestamp);
```

Implementation mirrors the detection block inside `run()` but omits the
`_shifter->shifter()` and `_delay.process()` calls. The `_track` flag must
be `true` for this to write anything into `_m_tune`.

### 3.3 Thread-safe result merging

The renderer owns its own `mx_tune` instance (`_scan_engine`). After scanning
a full clip, it publishes the collected `inpitch` data to the audio thread
through a lock-free structure (e.g., a `juce::AbstractFifo`-backed queue or a
simple `std::atomic<bool>` swap of a completed result set). The processor
picks this up in `processBlock` or via `juce::AsyncUpdater`.

---

## 4. Infrastructure Prerequisites

### 4.1 JUCE 8 Upgrade

**This must be completed and merged before any work in this plan begins.**

All JUCE version bump details, VST3 SDK removal, CMake version change, and
verification steps are documented separately in **`01_JUCE8_UPGRADE.md`**.
Start ARA work only from a `master` branch that is already on JUCE 8.

### 4.2 ARA SDK as Git Submodule

The ARA SDK is publicly available on GitHub. Add it as a submodule so all
environments get a consistent version:

```bash
git submodule add https://github.com/Celemony/ARA_SDK third_party/ARA_SDK
git submodule update --init --recursive
```

Add a submodule checkout step to both CI workflows (runs before CMake
configuration):

```yaml
- name: Checkout ARA SDK submodule
  run: git submodule update --init --recursive
```

### 4.3 ARA CMake Configuration

Before adding arguments, **verify the exact flag names** by searching in your
local JUCE 8 checkout:

```bash
grep -n "IS_ARA_EFFECT\|ARA_CONTENT_TYPES\|ARA_TRANSFORMATION" \
  third_party/JUCE/extras/Build/CMake/JUCEUtils.cmake
```

Expected additions to `juce_add_plugin()` in `CMakeLists.txt`:

```cmake
juce_add_plugin(MXTune
    # ... existing args unchanged ...
    IS_ARA_EFFECT              TRUE
    ARA_CONTENT_TYPES          "audio"          # verify exact string
    ARA_TRANSFORMATION_FLAGS   "clip_playback"  # verify exact string
)

juce_set_ara_sdk_path("${CMAKE_CURRENT_SOURCE_DIR}/third_party/ARA_SDK")
```

---

## 5. New Class Hierarchy

| New Class | Base Class | Responsibility |
|:---|:---|:---|
| `MXTuneDocumentController` | `juce::ARADocumentControllerSpecialisation` | ARA "brain" — manages host clip ↔ analysis data relationship; owns serialization |
| `MXTuneAudioModification` | `juce::ARAAudioModification` | Per-clip pitch correction data store (wraps `manual_tune` state) |
| `MXTunePlaybackRegionRenderer` | `juce::ARAPlaybackRegionRenderer` | **The data bridge.** Owns a scanning `mx_tune` instance; calls `scan()` from a background thread; merges results to the processor |

Modifications to existing classes:

```cpp
// JUCE/Source/PluginProcessor.cpp
// Actual class name is AutotalentAudioProcessor
class AutotalentAudioProcessor
    : public juce::AudioProcessor
    , public juce::ARAAudioProcessor   // ADD
```

```cpp
// JUCE/Source/PluginEditor.cpp
class AutotalentAudioProcessorEditor
    : public juce::AudioProcessorEditor
    , public juce::ARAEditorView       // ADD
```

---

## 6. Implementation Roadmap

### Phase 1 — ARA Handshake (~2 weeks)

**Goal:** Plugin appears in the DAW's ARA menu and loads without crashing.

- Add the ARA SDK submodule (§4.5) and CMake flags (§4.6).
- Implement `MXTuneDocumentController` with a minimal `createARAFactory()`.
- Wire `getStateInformation` / `setStateInformation` in `PluginProcessor.cpp`
  to include ARA document archiving alongside the existing tune-state
  serialization.
- Keep `processBlock` and the live `mx_tune` instance fully intact — this is
  the non-ARA fallback path, gated by `isBoundToARA()`.
- **Validation:** Plugin appears in Logic Pro's ARA menu AND in Studio One's
  ARA inspector. Load/unload several times without crash.

### Phase 2 — Offline Scanning via PlaybackRegionRenderer (~3 weeks)

**Goal:** Full-clip pitch data available without playback.

**Step A — Add `mx_tune::scan()`** (§3.2)

```cpp
// src/mx_tune.cpp — new method, detection only
void mx_tune::scan(const float* in, std::int32_t n, float timestamp)
{
    for (std::int32_t i = 0; i < n; i++)
    {
        float inpitch, conf;
        if (_detector->get_pitch(in[i], inpitch, conf))
        {
            float time_end   = timestamp + (float)i / (float)_sample_rate;
            float time_begin = time_end - _detector->get_time();
            manual_tune::pitch_node node{ conf, inpitch };
            _m_tune.set_inpitch(time_begin, time_end, node);
        }
    }
}
```

**Step B — Implement `MXTunePlaybackRegionRenderer::renderPlaybackRegion()`**

```cpp
void MXTunePlaybackRegionRenderer::renderPlaybackRegion(...)
{
    // 1. Get a reader for this audio source
    auto reader = playbackRegion.getAudioModification()
                                ->getAudioSource()
                                ->createReader();

    // 2. Compute clip-relative timestamp base
    double clipTimeBase = playbackRegion.getStartInAudioModificationTime();
    double sourceRate   = reader->sampleRate;

    // 3. Pull samples in chunks and feed the scan engine
    constexpr int CHUNK = 2048;
    AudioBuffer<float> chunk(1, CHUNK);
    int64_t pos = 0;

    while (pos < reader->lengthInSamples)
    {
        int n = std::min((int64_t)CHUNK, reader->lengthInSamples - pos);
        reader->read(&chunk, 0, n, pos, true, false);

        double timestamp = clipTimeBase + pos / sourceRate;
        _scan_engine->scan(chunk.getReadPointer(0), n, (float)timestamp);
        pos += n;
    }

    // 4. Trigger async merge into the live processor state
    _mergeResults();  // posts via juce::AsyncUpdater
}
```

**Threading rules:**
- `MXTunePlaybackRegionRenderer` owns `_scan_engine` — a dedicated `mx_tune`
  instance with `enable_track(true)` and auto-tune disabled.
- `_scan_engine` is **never** accessed from the audio thread.
- The live `AutotalentAudioProcessor::_mx_tune` is **never** accessed from
  the renderer thread.
- If `ARAAudioSource::getSampleRate()` ≠ the project sample rate, resample via
  `juce::ResamplingAudioSource` before calling `scan()`. `libsamplerate` is
  already linked — the existing `pitch_shifter_smb` resampling code is a
  reference for the API.

### Phase 3 — Enhanced Metadata (~3 weeks)

**Goal:** Store vibrato shape and transient onsets in the ARA document model.

- **Vibrato extraction:** Post-process the pitch curve stored in `_m_tune`
  after scanning to detect periodic oscillations. Store depth and rate per
  note region inside `MXTuneAudioModification`.
- **Transient detection:** Use aubio's onset detection (already linked) on
  the same `ARAAudioSourceReader` pass to mark syllable boundaries. These
  become snap points for Phase 4's time-stretch drag handles.

### Phase 4 — UI Integration (~2 weeks)

**Goal:** Wire the ARA editor view into the modernized UI (see
`04_UI_MODERNIZATION/04_UI_MODERNIZATION.md`).

- Implement `ARAEditorView::onUpdatePlaybackRegionProperties()` to respond to
  host zoom and clip changes, driving repaints of the pitch grid.
- Ensure the `MXTuneLookAndFeel` and layout system from Plan 04 are applied
  inside the ARA editor view without duplication.
- **New animation module:** Use JUCE 8's cubic-bezier `juce::Animator` for
  smooth note-snap transitions.

---

## 7. Host Compatibility

| Host | ARA Support | Behaviour |
|:---|:---|:---|
| Logic Pro, Studio One | Full ARA 2.0 | Primary targets; full offline scan |
| Reaper | Full ARA 2.0 | Secondary validation target |
| Ableton Live 12+ | Limited ARA | Non-ARA `processBlock` fallback required for stability |
| FL Studio, others | None | Automatic fallback to real-time play-to-scan via `isBoundToARA()` |

The fallback path (`processBlock` + `_track` + live `mx_tune`) must remain
fully functional throughout all phases.

---

## 8. Risk Register

| Risk | Phase | Mitigation |
|:---|:---|:---|
| CMake flag names differ from plan | Phase 1 build | Run the `grep` in §4.6 against actual JUCE 8 `JUCEUtils.cmake` before writing CMake |
| Sample rate mismatch (ARA source ≠ project) | Phase 2 | Check `ARAAudioSource::getSampleRate()` before every reader session; resample if needed |
| Data race on `_m_tune` | Phase 2 | Renderer uses isolated `_scan_engine`; merge only via `AsyncUpdater` |
| `mx_tune::scan()` diverges from `run()` detection logic | Phase 2 onward | Keep both paths calling the same `_detector->get_pitch()` — no duplicated algorithm code |
| ARA behaviour differences between DAWs | Phase 1 validation | Test selection-based vs. track-based ARA in Logic Pro AND Reaper before Phase 2 |

---

## 9. CI / Build Changes Summary

Changes required specifically for ARA (on top of the already-merged JUCE 8 upgrade):

| File | Change |
|:---|:---|
| `.github/workflows/release.yml` | Add submodule checkout step for `third_party/ARA_SDK` |
| `.github/workflows/pr-build.yml` | Same as above |
| `CMakeLists.txt` | Add `IS_ARA_EFFECT` + `juce_set_ara_sdk_path` to `juce_add_plugin` |
| `.gitmodules` | Add `third_party/ARA_SDK` submodule entry |
| `src/mx_tune.h` / `src/mx_tune.cpp` | Add `scan()` method (§3.2) |

---

## 10. Future Expansions (Post-ARA Stable)

- **Time-stretching:** `librubberband` is already linked in `src/`. Once ARA
  clip boundaries are stable, expose drag handles in the editor that call
  `RubberBandStretcher` directly rather than through `processBlock`.
- **Polyphonic ghost notes:** ARA full-document access allows reading
  correction data from sibling MXTune instances in the same session, enabling
  neighbouring vocal lines as semi-transparent pitch-grid overlays.

---

## 11. Immediate Next Steps (Ordered)

1. **Complete the JUCE 8 upgrade first** — follow `01_JUCE8_UPGRADE.md` in
   full and merge to `master` before touching anything below.
2. Run `git submodule add https://github.com/Celemony/ARA_SDK third_party/ARA_SDK`.
3. Verify the ARA CMake flag names against JUCE 8's `JUCEUtils.cmake` — see §4.3.
4. Create branch `feature/ara-phase1` off the JUCE-8 `master` and implement
   the Phase 1 handshake.
