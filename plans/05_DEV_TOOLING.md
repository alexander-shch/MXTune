# Dev Tooling — Local Testing Environment

> Prerequisite: `01_JUCE8_UPGRADE.md` merged to `master`.
> Implement this before starting ARA work — it makes iteration significantly faster.

---

## Goal

Replace the current manual build→copy→restart-DAW loop with a one-command
dev cycle. Target: code change to plugin running in under 5 seconds.

**Current workflow (painful):**
1. Build
2. Manually copy plugin bundle to `/Library/Audio/Plug-Ins/VST3/`
3. Quit Reaper
4. Reopen Reaper (full plugin scan on startup)
5. Re-add plugin to track

**Target workflow:**
1. `./scripts/dev.sh` → build, auto-copy, relaunch host with plugin pre-loaded

---

## Changes

### 1. Auto-copy on build (`CMakeLists.txt`)

Add `COPY_PLUGIN_AFTER_BUILD TRUE` to `juce_add_plugin()`:

```cmake
juce_add_plugin(MXTune
    # ... existing args ...
    COPY_PLUGIN_AFTER_BUILD TRUE
)
```

JUCE will copy the built bundle to the system plugin directory after every
successful build. No manual copy step ever again.

> macOS target: `/Library/Audio/Plug-Ins/VST3/` and `/Library/Audio/Plug-Ins/Components/`
> Windows target: `C:\Program Files\Common Files\VST3\`

### 2. JUCE AudioPluginHost

Build the AudioPluginHost from the JUCE extras:

```bash
cmake --build third_party/JUCE/extras/AudioPluginHost --config Release
```

Or add it as a CMake target so it builds alongside the plugin:

```cmake
add_subdirectory(third_party/JUCE/extras/AudioPluginHost EXCLUDE_FROM_ALL)
```

Save a `.filtergraph` preset file at `dev/mxtune_dev.filtergraph` with:
- MXTune loaded on a channel
- A test audio file routed through it
- Output to system audio

The host relaunches in ~1 second (no sample library scan, no project).

### 3. `scripts/dev.sh`

```bash
#!/bin/bash
set -e

# Build (COPY_PLUGIN_AFTER_BUILD handles the copy)
cmake --build build --target MXTune_VST3 --config Debug

# Relaunch AudioPluginHost with the saved preset
pkill -x "AudioPluginHost" 2>/dev/null || true
open -a AudioPluginHost dev/mxtune_dev.filtergraph
```

Usage: `./scripts/dev.sh`

For AU format (macOS): add `MXTune_AU` to the build target and run
`killall -9 AudioComponentRegistrar` before relaunching the host to force
a re-registration of the AU component.

---

## When to Use What

| Scenario | Tool |
|:---------|:-----|
| UI changes, crash testing, pitch grid rendering | AudioPluginHost |
| Standalone-compatible feature work | Standalone app target |
| ARA-specific behaviour (clip analysis, DAW document model) | Reaper or Logic Pro |
| Release validation | Full DAW (Reaper, Logic Pro, Studio One) |

---

## Branch

No dedicated branch — these are build-system changes committed directly to
`master` immediately after the JUCE 8 upgrade is merged.
