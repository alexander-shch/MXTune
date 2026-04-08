# ARA Implementation Plan

## Purpose
This plan describes the changes needed to migrate MXTune from processBlock-based buffer-limited scanning to JUCE ARA-based full-track audio access and offline analysis.

## Goals
- Enable ARA support in the plugin build
- Replace the current ring-buffer analysis path with ARA document/audio source access
- Support full-clip or full-track analysis regardless of playhead position
- Keep UI update flow responsive by using background analysis and ARA document change notifications

## Prerequisites
- Confirm JUCE ARA support is available in the current JUCE modules and build environment
- Confirm the ARA SDK path can be set for the build system
- Ensure the plugin target is compatible with ARA in the chosen formats (VST3, AU, etc.)
- Review current `JUCE/JuceLibraryCode/JucePluginDefines.h` settings for `JucePlugin_Enable_ARA` and ARA content flags

## Key concepts
- `ARADocumentControllerSpecialisation`: plugin-side class responsible for creating ARA documents and managing host interactions
- `ARADocumentController::createARAFactory()`: global entry point required by JUCE ARA plugins
- `juce::ARAAudioSource`: provides access to full audio source data for scanning, not only live processBlock buffers
- ARA document lifecycle: capture, clone, edit, render, and clean up document data in the plugin

## Implementation steps

### 1. Enable ARA support in the build
- Set `JucePlugin_Enable_ARA` to `1` in `JUCE/JuceLibraryCode/JucePluginDefines.h`
- Add or configure `JucePlugin_ARAContentTypes` with supported content types, e.g. `jura::ARAContentType::audio` or equivalent
- Add `JucePlugin_ARATransformationFlags` if needed for clip playback and audio read access
- Update `CMakeLists.txt` to set the ARA SDK path using `juce_set_ara_sdk_path()` or equivalent macro
- Ensure the JUCE module list includes any ARA-related modules required by JUCE 8/9+ (e.g. `juce_audio_utils` or `juce_ara` if separate)

### 2. Add ARA document controller integration
- Create a new source file for the ARA specialization, e.g. `ARADocumentController.cpp` / `.h`
- Subclass `juce::ARADocumentControllerSpecialisation<AutotalentAudioProcessor>` or equivalent depending on JUCE version
- Implement required virtual methods:
  - `createARAFactory()` global entry point
  - `getDocumentName()`
  - `createDocumentController()`
  - document state and render callbacks as required by JUCE ARA
- Ensure the document controller is instantiated and connected with the plugin processor

### 3. Expose ARA-aware audio source reading
- Replace or augment `AutotalentAudioProcessor::analyze_current_audio()` to accept a `juce::ARAAudioSource` rather than a raw ring buffer
- Add a new ARA callback or document render stage where the full source audio is available for analysis
- Use `ARAAudioSource::createReader()` or `ARAAudioSource::getAudioDataForRange()` to read clip audio in contiguous blocks
- Convert the read audio frames into the existing `mx_tune` scan path with proper channel handling

### 4. Implement offline/full-track scanning
- Design a scan task that reads the entire clip or selected region before playback
- Use a background thread or `juce::ThreadPoolJob` to avoid blocking the UI or audio thread
- Keep the UI responsive by signaling completion via `MessageManagerCallback` or `AsyncUpdater`
- Continue using existing pitch/note storage structures, but populate them from the ARA source data instead of the live buffer

### 5. Update UI and control flow
- Keep the existing `Update` button or add a dedicated ARA scan trigger
- When triggered, start the ARA analysis task and disable UI controls as needed
- On completion, refresh note/pitch display using `manual_tune` data updated by the scan
- If the host changes the ARA document or audio source, invalidate cached analysis and rescan as needed

## Important gotchas and potential issues
- ARA requires a compatible host and plugin registration; the plugin may still need to support non-ARA hosts gracefully
- ARA plugin entry points and ARA controller creation must match JUCE version-specific APIs exactly
- `processBlock()` may still be called for host playback; the plugin must continue audio processing correctly while the ARA scan is performed separately
- ARA audio source access can be expensive if reading an entire track; use chunked reads and avoid repeated full-track reads
- Thread-safety: the analysis task must not access audio-engine state from the audio thread, and UI updates must happen on the message thread
- Sample rate/buffer size differences: the ARA audio source may expose a different sample rate than the current playback context; ensure `mx_tune` receives consistent sample-rate-aligned data
- Clip/region boundaries: the ARA source may present audio as multiple clips or sources; the scan implementation should normalize this into a single contiguous analysis stream if the plugin expects it
- State sync: host and ARA document changes can occur asynchronously; add defensive checks for invalidated documents during scan operations
- Build configuration mismatch: ARA SDK path or missing module support will lead to build failures that may be subtle and platform-specific
- Plugin metadata and format restrictions: not every format supports ARA or ARA document-based workflows equally

## What to be aware of during implementation
- Preserve existing non-ARA functionality for hosts that do not support ARA unless the plugin intentionally becomes ARA-only
- Keep the current scanning logic in place long enough for comparison and fallback testing
- Do not assume the ARA document contains audio for the entire project; the host may expose only selected clips or affected ranges
- Validate that `ARAAudioSource` returns samples in the expected channel layout and that mono/stereo handling stays consistent
- Use JUCE logging or debug output while first reading ARA data to verify source length, sample rate, channels, and frame ranges
- Test with multiple hosts, especially those known to support ARA (e.g. Logic Pro, Pro Tools, Studio One) and non-ARA hosts
- Consider how this change affects automation, undo/redo, and plugin state serialization
- Design the data flow so that ARA-driven scan results can be reused for later editing operations

## Future ARA-powered feature expansions
These features should be built on top of the ARA scan/data pipeline once full-track audio access is available.

### Vibrato correction / modulation control
- Use ARA pitch and time data to detect vibrato depth and rate from the scanned audio
- Provide controls to reduce, preserve, or exaggerate vibrato by adjusting pitch bend and envelope shape
- Keep vibrato correction separate from pitch correction so the effect can be applied selectively per phrase or syllable
- Ensure the same scan pipeline can produce both raw pitch information and optional vibrato modulation metadata

### Time changes and phrase lengthening
- Use full-clip ARA audio access to detect phrase boundaries and rhythmic anchors before editing
- Allow the user to make a specific word or syllable longer by time-stretching the selected region
- Implement this as a separate transformation step after pitch analysis, using ARA data to align edits to musical timing
- Preserve natural timing by respecting phrase context and limiting stretch amount within musical and vocal constraints

### Tempo detection and beat alignment
- Add a tempo scan stage that analyzes the current beat grid from the section being scanned
- Use ARA document/beat metadata or derived transient/energy analysis to identify BPM and downbeats
- Expose a tempo set parameter so the plugin can shift vocal timing toward the detected beat
- Use beat alignment to make vocals perform more on time while preserving expressive timing in non-critical regions

### Workflow considerations
- Keep the new ARA features optional so the plugin remains usable for simple pitch-correction tasks
- Store scan/analysis results in a reusable structure so pitch, vibrato, and timing edits share the same underlying data
- Design the UI to let users choose between "scan only," "pitch correction," and "timing/tempo" workflows
- Provide clear indicators when ARA-powered editing is active versus when host playback is simply live-processed

## Testing and validation
- Verify build passes with ARA enabled on macOS and any other supported platforms
- Validate ARA document creation and host recognition in an ARA-capable DAW
- Compare scan results between the current ring-buffer approach and the new ARA-based full-track scan
- Confirm UI refresh occurs after ARA scan completion and note/pitch display uses the new data
- Test playback through the plugin in both ARA and non-ARA hosts to ensure audio processing remains intact
- Log or assert document/audio source durations and read ranges during initial development
- Validate each new feature stage separately:
  - pitch scan only
  - vibrato reduction/addition
  - syllable/phrase lengthening
  - tempo/beat alignment

## Implementation roadmap: phases and milestones

### Phase 1: ARA Build & Basic Integration (~1-2 weeks)
**Objective:** Enable ARA in the build and create a working ARA document controller.

- Milestone 1.1: Update build config (CMakeLists.txt, ARA SDK path, plugin defines)
- Milestone 1.2: Create ARA document controller specialization and factory entry point
- Milestone 1.3: Verify plugin builds and loads in an ARA host (Logic Pro, Studio One, etc.)
- Milestone 1.4: Test non-ARA hosts still work (fallback mode)

**Deliverable:** Plugin registers as ARA-capable in host; document/audio source wiring is functional.

### Phase 2: Full-Track Analysis Pipeline (~2-3 weeks)
**Objective:** Replace ring-buffer scanning with ARA-based offline full-track analysis.

- Milestone 2.1: Implement ARA audio source reading and chunked sample access
- Milestone 2.2: Port current `analyze_current_audio()` logic to use ARA source data
- Milestone 2.3: Add background thread task pool for offline scanning
- Milestone 2.4: Implement UI callback / async update so results display correctly
- Milestone 2.5: Validate pitch detection output matches or improves vs. ring-buffer approach

**Deliverable:** Update button scans full clip, populates pitch grid correctly, UI updates on completion.

### Phase 3: Vibrato detection & correction (~2-3 weeks)
**Objective:** Analyze vibrato characteristics from full-track scan and provide reduction/control.

- Milestone 3.1: Add vibrato depth/rate detection to pitch analysis pipeline
- Milestone 3.2: Store vibrato metadata alongside pitch in manual_tune data structure
- Milestone 3.3: Add vibrato reduction slider to UI
- Milestone 3.4: Implement vibrato modulation in the pitch shifter or auto-tune stage
- Milestone 3.5: Test vibrato suppression and add-back workflows

**Deliverable:** User can adjust vibrato independently of main pitch correction.

### Phase 4: Time stretch & phrase/syllable editing (~3-4 weeks)
**Objective:** Allow word or syllable lengthening using full-track timing information.

- Milestone 4.1: Analyze phrase boundaries and transients in the ARA source
- Milestone 4.2: Add syllable/phrase selection UI (click/drag regions on pitch grid)
- Milestone 4.3: Implement time-stretch for selected regions (preserve naturalness)
- Milestone 4.4: Synchronize with audio output and Test with various vocal styles

**Deliverable:** User can select a syllable or word and make it longer/shorter while maintaining vocal quality.

### Phase 5: Tempo detection & beat alignment (~2-3 weeks)
**Objective:** Detect beat grid and align vocal timing to the detected tempo.

- Milestone 5.1: Implement beat/transient detection from the ARA audio source
- Milestone 5.2: Extract BPM and downbeat information from the analyzed region
- Milestone 5.3: Add tempo/beat-alignment UI controls
- Milestone 5.4: Integrate beat-aligned timing correction into playback
- Milestone 5.5: Test with various tempos and musical styles

**Deliverable:** Plugin can detect and adjust to the current beat; vocalists perform more on time.

### Phase 6: Polish, optimization & multi-host testing (~2-3 weeks)
**Objective:** Ensure all features work smoothly across hosts and edge cases are handled.

- Milestone 6.1: Performance profiling and optimization (memory, CPU, I/O)
- Milestone 6.2: Test on Logic Pro, Pro Tools, Studio One, and REAPER
- Milestone 6.3: Handle edge cases (empty clips, very long tracks, session changes)
- Milestone 6.4: Documentation for users and developers
- Milestone 6.5: Fix reported issues and finalize release

**Deliverable:** Stable, performant ARA plugin ready for distribution.

## Work estimation summary
- **Total estimated effort:** 12–18 weeks (3–4.5 months) for all phases
- **Phase 1 (ARA build):** critical, blocks everything else; should be completed first
- **Phase 2 (full-track scanning):** high value, addresses the core limitation
- **Phases 3–5:** can be prioritized by user feedback; vibrato is likely most impactful
- **Phase 6:** ongoing; begin after Phase 2 solid, parallel to Phases 3–5

## References and useful files
- `JUCE/JuceLibraryCode/JucePluginDefines.h`
- `CMakeLists.txt`
- `src/PluginProcessor.cpp`
- `src/PluginGui.cpp`
- `third_party/JUCE/docs/ARA.md`
- JUCE ARA examples or templates in the JUCE module tree
