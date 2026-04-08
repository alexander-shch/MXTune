Here is the complete, single-file Markdown script. You can copy this entire block and save it as `TEST_CASES.md` in your project root. 

It is designed to be a "living document" that bridges the gap between your **C++ logic (Aubio/SoundTouch)** and the **ARA 2/JUCE** container requirements.

---

```markdown
# MXTune: Comprehensive ARA 2 & Vocal DSP Test Suite

This document defines the mandatory validation scenarios for **MXTune**. All major commits must pass the "Core Logic" tests, and all releases must pass the "ARA Timeline" gauntlet.

---

## 1. Unit Tests: Core DSP (DAW-Independent)
*Target: `MXTuneScanner` / `AubioWrapper`*
*Run these via the `MXTuneConsoleTester` target for rapid iteration.*

| ID | Scenario | Input Asset | Expected Result |
| :--- | :--- | :--- | :--- |
| **DSP-01** | **Sine Sweep** | `sweep_20_to_2k.wav` | Detector tracks frequency with < 0.1Hz deviation. |
| **DSP-02** | **Static Note** | `vocal_pure_A440.wav` | Resulting pitch map is a perfectly horizontal line at 440Hz. |
| **DSP-03** | **Low-End Fry** | `vocal_metal_fry_B1.wav` | No octave-jumping; stays locked to the fundamental (~31Hz). |
| **DSP-04** | **Noise Rejection** | `white_noise_0db.wav` | Detector reports "Unpitched" or 0Hz; no ghost notes generated. |
| **DSP-05** | **Sibilance** | `vocal_s_sh_t.wav` | High-frequency transients do not trigger pitch detection. |

---

## 2. Integration Tests: ARA 2 Source Management
*Target: `ARAAudioSource` & Threading*
*Test in: JUCE AudioPluginHost (ARA mode) or REAPER.*

| ID | Scenario | Test Action | Expected Result |
| :--- | :--- | :--- | :--- |
| **ARA-01** | **Instant Load** | Drag 10m WAV to track. | Grid/Waveform populate in background in < 3s. |
| **ARA-02** | **File Swap** | Replace source file in DAW. | Old pitch map is purged; new scan triggers automatically. |
| **ARA-03** | **Concurrency** | Move playhead during scan. | Audio playback is smooth; background scan is uninterrupted. |
| **ARA-04** | **Sample Rate** | 48k clip in 44.1k project. | ARA reader correctly handles resampling for the pitch grid. |

---

## 3. Timeline & Layout Tests (The "REAPER" Gauntlet)
*Target: `ARAPlaybackRegion` & UI Rendering*

| ID | Scenario | Test Action | Expected Result |
| :--- | :--- | :--- | :--- |
| **TIM-01** | **Item Split** | Split item in REAPER ('S'). | Both items share the same source data; no redundant scanning. |
| **TIM-02** | **The Crossfade** | Overlap two clips. | UI shows overlapping pitch lines (A fading out, B fading in). |
| **TIM-03** | **Item Offset** | Slip-edit vocal start. | Pitch lines remain locked to the absolute audio content time. |
| **TIM-04** | **Reverse Item** | Reverse audio in REAPER. | Pitch grid flips horizontally or re-scans to match reversed audio. |
| **TIM-05** | **Empty Space** | 30s gap between items. | No memory allocated for the silent gap in the pitch map. |

---

## 4. Vocal-Specific Validation
*Target: Musicality & SoundTouch Processing*

| ID | Scenario | Test Action | Expected Result |
| :--- | :--- | :--- | :--- |
| **VOC-01** | **Operatic Vibrato** | Wide pitch modulation. | Grid shows smooth sine-curve; no "stepped" artifacts. |
| **VOC-02** | **Formant Shift** | Shift vocal +4 semitones. | Singer sounds higher but remains "human" (SoundTouch compensation). |
| **VOC-03** | **Micro-Scoops** | Singer sliding into a note. | The "scoop" is visually represented on the grid as a curve. |
| **VOC-04** | **Metal Screams** | High-distortion vocals. | Detector ignores harmonic grit and identifies the core pitch. |

---

## 5. UI/UX & Rendering Stress Tests
*Target: `PluginEditor` & Graphics Performance*

- [ ] **Zoom Stress:** Zoom out to view 1 hour of audio. Waveform rendering must stay > 60fps.
- [ ] **High DPI:** Verify grid alignment on 4K/Retina displays.
- [ ] **Undo/Redo:** Perform 10 pitch edits, then `Ctrl+Z` all of them. Audio must revert perfectly.
- [ ] **Offline Render:** "Render to File" at 50x speed. Verify no glitches in the output file.

---

## 6. Automated "Torture" Checklist
*A list of things that usually break ARA plugins:*

1. [ ] **Multiple Instances:** 10 tracks of MXTune running simultaneously.
2. [ ] **Bypass/Unbypass:** Toggling the plugin while the playhead is moving.
3. [ ] **Plugin Removal:** Removing MXTune while a background scan is active.
4. [ ] **Automation:** Automating the "Correction Amount" while the playhead crosses a region boundary.

---

### Dev Monologue:
> Keep the `TestAssets` folder clean. If a specific vocal take causes a crash in REAPER, save that clip, add it to the unit tests, and don't touch the ARA code again until the **ConsoleTester** can process that clip without barfing.
```
