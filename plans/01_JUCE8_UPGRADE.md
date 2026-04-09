# JUCE 8 Upgrade Plan

> Prerequisite for ARA work. Must be completed and merged to `master` before
> any ARA implementation begins. See `02_ARA_IMPLEMENTATION.md`.

---

## Goal

Replace JUCE 7.0.5 with JUCE 8.0.12 while keeping the existing plugin
behaviour completely unchanged. This is a build-system and dependency update
only — no DSP or UI logic changes.

**Done when:** Clean three-platform CI build passes, and the plugin loads and
processes audio correctly in Logic Pro and Reaper with no regression.

---

## 1. Branch Strategy

Work on a dedicated branch: `feature/juce8-upgrade` off `master`.  
Do not merge to `master` until all verification steps in §5 pass.

---

## 2. Changes Required

### 2.1 Update JUCE Version in CI

JUCE is downloaded at build time (not a submodule). Update the version string
in **both** workflow files:

**Files:** `.github/workflows/release.yml` and `.github/workflows/pr-build.yml`

```yaml
# Before — appears in download URL and cache key
juce-7.0.5-${{ runner.os }}

# After
juce-8.0.12-${{ runner.os }}
```

Full download URL pattern to update:
```
# Before
https://github.com/juce-framework/JUCE/releases/download/7.0.5/juce-7.0.5-<platform>.zip

# After
https://github.com/juce-framework/JUCE/releases/download/8.0.12/juce-8.0.12-<platform>.zip
```

### 2.2 Remove the Standalone VST3 SDK

JUCE 8 bundles **VST3 SDK 3.8.0** internally. The repo's `VST3_SDK/` directory
(v3.7.1_build_50) will conflict with it.

- Delete `VST3_SDK/` from the repo root.
- Remove the VST3 SDK download step and its cache entry from both CI workflows.
- Remove any `-DVST3_SDK_ROOT=...` CMake flags or include paths that reference
  `VST3_SDK/`.

### 2.3 Bump CMake Minimum Version

JUCE 8's CMake integration requires **CMake 3.22+**.

```cmake
# CMakeLists.txt, line 3 — change from:
cmake_minimum_required(VERSION 3.15)
# to:
cmake_minimum_required(VERSION 3.22)
```

GitHub-hosted `ubuntu-latest`, `macos-latest`, and MSYS2 on Windows all
ship 3.22+ — no runner changes needed.

Local check before committing:
```bash
cmake --version   # must be >= 3.22
# if not: brew upgrade cmake
```

### 2.4 Handle JUCE 8 Breaking Changes

Grep the `JUCE/Source/` directory for known JUCE 8 breaking changes before
attempting a build:

```bash
# TrackProperties::colour was removed; replaced with colourARGB
grep -rn "TrackProperties::colour" JUCE/Source/
```

Replace any hits with `colourARGB`. There may be zero hits in this codebase —
this is a precautionary check.

---

## 3. What Does NOT Change

- C++ standard stays at C++17 (already meets JUCE 8's minimum requirement).
- All DSP logic in `src/` is untouched.
- All plugin formats (VST3, AU, Standalone) remain as-is.
- No ARA-related CMake flags are added in this upgrade — that is Phase 1 of
  the ARA plan.

---

## 4. CI Changes Summary

| File | Change |
|:---|:---|
| `.github/workflows/release.yml` | `7.0.5` → `8.0.12` in download URL and cache key; remove VST3 SDK download and cache steps |
| `.github/workflows/pr-build.yml` | Same as above |
| `CMakeLists.txt` | `cmake_minimum_required` 3.15 → 3.22 |
| `VST3_SDK/` | **Delete entire directory** |

---

## 5. Verification Checklist

All items must pass before merging to `master`.

**Build:**
- [ ] Clean build on macOS (Xcode 15+)
- [ ] Clean build on Ubuntu (GCC, `ubuntu-latest`)
- [ ] Clean build on Windows (MSYS2)
- [ ] All existing unit tests pass (`tests/`)

**Runtime:**
- [ ] VST3 loads in Logic Pro — no crash on load/unload
- [ ] AU loads in Logic Pro — processes audio, tuning behaviour unchanged
- [ ] VST3 loads in Reaper — no crash on load/unload
- [ ] Standalone app launches on macOS
- [ ] No latency regression (compare reported latency samples before/after)

**If CI fails:** Do not attempt to fix by patching JUCE internals. Identify
whether the failure is a JUCE 8 breaking change in `JUCE/Source/` or a
CMakeLists.txt issue, fix it there, and re-run.

---

## 6. After This Merge

Once on `master`, the repo is ready for `feature/ara-phase1`. Do not start
ARA work on a branch based on the old JUCE 7 state.
