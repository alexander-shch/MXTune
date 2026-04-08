# MXTune

[![Build and Release](https://github.com/alexander-shch/MXTune/actions/workflows/release.yml/badge.svg)](https://github.com/alexander-shch/MXTune/actions/workflows/release.yml)

An open-source auto-tune / pitch correction audio plugin (VST3/AU) with real-time pitch detection and correction to a user-defined musical scale or MIDI-driven note mapping.

See the [CHANGELOG.md](./CHANGELOG.md) for a history of features and bugfixes.

## Credits

- **Original author:** [Liu An Lin (liuanlin-mx)](https://github.com/liuanlin-mx/MXTune) — core pitch detection and correction algorithm, based on Tom Baran's [Autotalent](http://web.mit.edu/tbaran/www/autotalent.html) and [TalentedHack](http://code.google.com/p/talentledhack/)
- **macOS build structure:** [Hammond95](https://github.com/Hammond95/MXTune) — macOS CMake setup, Projucer integration, and macOS 15+ compatibility patch
- JUCE is licensed under the GPL v3 or a commercial license. See [juce.com](https://juce.com) for more details.

## Download

> Stable releases for macOS, Windows, and Linux are published automatically on every merge to `master`.

| Platform | Download |
|---|---|
| macOS | [Latest release](https://github.com/alexander-shch/MXTune/releases/latest) |
| Windows | [Latest release](https://github.com/alexander-shch/MXTune/releases/latest) |
| Linux | [Latest release](https://github.com/alexander-shch/MXTune/releases/latest) |

All releases: [github.com/alexander-shch/MXTune/releases](https://github.com/alexander-shch/MXTune/releases)

## Installation

### macOS
Extract the `.zip`:
- **VST3**: move `mx_tune-*.vst3` to `/Library/Audio/Plug-Ins/VST3/`
- **AU**: move `mx_tune-*.component` to `/Library/Audio/Plug-Ins/Components/`

Rescan plugins in your DAW.

### Windows
Extract the `.zip`, move the `.vst3` folder to `C:\Program Files\Common Files\VST3\`.

### Linux
Extract the `.zip`, move the `.vst3` folder to `~/.vst3/` or `/usr/lib/vst3/`.

---

## Usage

### Hot keys

- `alt + mouse wheel` — x zoom
- `ctrl + mouse wheel` — y zoom
- `mouse wheel` or `W / S` — y move
- `shift + mouse wheel` or `A / D` — x move
- `left button drag` — add note
- `right button` — delete note

---

## Building from source

### macOS
Prerequisites: Homebrew, Xcode command-line tools, CMake, Git.
```bash
./build_macos.sh
```

### Windows (MSYS2 / MinGW-w64)
Prerequisites: [MSYS2](https://www.msys2.org/), MINGW64 environment.
```bash
./build_windows.sh
```

### Linux
Prerequisites: GCC, CMake, pkg-config, and development headers for FFTW, SoundTouch, etc.
```bash
./build_linux.sh
```

---

## Developer Notes

This project is built using **JUCE 7** and **CMake**. It bridges legacy C-based pitch algorithms with modern C++ plugin wrappers.

### Architecture & Dependencies
The plugin relies on several high-performance audio libraries:
- **RubberBand**: High-quality time-stretching and pitch-shifting.
- **SoundTouch**: Alternative pitch-shifting engine.
- **Aubio**: Real-time pitch detection (YIN/FFT based).
- **FFTW3**: Fast Fourier transforms for spectral processing.

### Platform-Specific Details

#### macOS (Self-Contained Bundles)
Standard macOS plugins are often dynamically linked to Homebrew libraries, which causes them to crash on other users' machines. Our `build_macos.sh` script handles this:
1. **Framework Bundling**: Copies `.dylib` files from `/opt/homebrew/` into the plugin's `Contents/Frameworks/` directory.
2. **RPath Fixing**: Uses `install_name_tool` to change load paths to `@loader_path/../Frameworks/`.
3. **Deep Signing**: Signs internal dylibs individually before performing a "deep" signature on the bundle.

#### Windows (Static Linking)
On Windows, we prefer **static linking** to avoid "missing DLL" issues. The `CMakeLists.txt` is configured to link dependencies statically when building with MinGW/MSYS2.

#### Linux
Linux builds generate a standard `.so` binary within a `.vst3` bundle structure, following the VST3 SDK specification.

### JUCE & macOS 15 Compatibility
JUCE 7.0.5 has a known issue with `CGWindowListCreateImage` on macOS 15 Sequoia. Our build script includes a patcher that automatically guards this call in the JUCE source code before building.

---

## Troubleshooting

If the plugin fails to load or shows as "incompatible":

1. **DAW Cache**: Most DAWs (like REAPER, Ableton, or Logic) cache failed plugin scans. You must **"Clear Cache and Rescan"** in your DAW preferences after an update.
2. **macOS Permissions**: If you get a "Developer cannot be verified" message, go to **System Settings > Privacy & Security** and click "Allow Anyway".
3. **macOS Validation**: Try running `auval -v aufx MXTn Manu` in a terminal to see if the system reports any signature or dependency errors.
4. **Linux Dependencies**: Ensure you have installed the required runtime libraries (FFTW3, SoundTouch, etc.) as the Linux build is currently dynamically linked.
