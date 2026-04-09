# MXTune — Plans Index

Plans must be executed in order. Each plan is a hard prerequisite for the next.

---

## Execution Order

| # | Plan | Branch | Status | Est. Duration |
|:--|:-----|:-------|:-------|:--------------|
| 1 | [JUCE 8 Upgrade](01_JUCE8_UPGRADE.md) | `feature/juce8-upgrade` | Pending | ~1 week |
| 2 | [ARA Implementation](02_ARA_IMPLEMENTATION.md) | `feature/ara-phase*` | Pending | ~12 weeks |
| 3 | [ARA Testing](03_ARA_TESTING.md) | runs alongside plan 2 | Pending | ongoing |

---

## Plan Summaries

### 01 — JUCE 8 Upgrade
**Target:** Replace JUCE 7.0.5 with JUCE 8.0.12. Build-system change only — no DSP or UI logic touched.

Key changes:
- Update JUCE download URL in both CI workflows
- Remove `VST3_SDK/` directory (JUCE 8 bundles VST3 3.8.0 internally)
- Bump `cmake_minimum_required` from 3.15 → 3.22

**Done when:** Clean three-platform CI build, plugin loads and tunes correctly in Logic Pro and Reaper.

---

### 02 — ARA Implementation
**Target:** Migrate from real-time `processBlock` scanning to full-clip offline analysis via JUCE 8 ARA.
**Prerequisite:** Plan 01 merged to `master`.

Four phases:
| Phase | Goal | Duration |
|:------|:-----|:---------|
| 1 — ARA Handshake | Plugin registers as ARA in Logic Pro / Studio One | ~2 weeks |
| 2 — Offline Scanning | Full-clip pitch data via `ARAAudioSourceReader`, no playback needed | ~3 weeks |
| 3 — Enhanced Metadata | Vibrato extraction and transient detection stored in ARA document model | ~3 weeks |
| 4 — Modernized UI | Hardware-accelerated pitch grid (Metal/Direct2D), smooth animations | ~4 weeks |

Key pre-implementation code changes required in `src/`:
- Add `mx_tune::scan()` — detection-only path, no pitch shifting
- `MXTunePlaybackRegionRenderer` owns its own `mx_tune` scan instance; results merged to audio thread via `AsyncUpdater`

Non-ARA hosts (Ableton, FL Studio) fall back to existing `processBlock` path automatically via `isBoundToARA()`.

---

### 03 — ARA Testing
**Target:** Validate correctness and stability of the ARA implementation across DSP edge cases, host behaviours, and UI stress conditions.
**Runs:** Alongside and after Plan 02 phases.

Test categories:
- **DSP unit tests** — sine sweep, static note, noise rejection, sibilance (DAW-independent)
- **ARA source management** — instant load, file swap, concurrency, sample rate mismatch
- **Timeline layout** — item split, crossfade, slip edit, reverse, empty gaps (Reaper gauntlet)
- **Vocal-specific** — vibrato, formant shift, micro-scoops, metal screams
- **UI/UX stress** — zoom, high DPI, undo/redo, offline render, multi-instance torture tests
