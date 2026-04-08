# MXTune

[![Build and Release](https://github.com/alexander-shch/MXTune/actions/workflows/release.yml/badge.svg)](https://github.com/alexander-shch/MXTune/actions/workflows/release.yml)

An open-source auto-tune / pitch correction audio plugin (VST3) with real-time pitch detection and correction to a user-defined musical scale or MIDI-driven note mapping.

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

### Installation

**macOS** — extract the `.zip`:
- VST3: move `mx_tune-*.vst3` to `/Library/Audio/Plug-Ins/VST3/`
- AU: move `mx_tune-*.component` to `/Library/Audio/Plug-Ins/Components/`

Rescan plugins in your DAW.

**Windows** — extract the `.zip`, move `mx_tune-*.vst3` to `C:\Program Files\Common Files\VST3\`, rescan in your DAW.

**Linux** — extract the `.zip`, move `mx_tune-*.vst3` to `~/.vst3/` or `/usr/lib/vst3/`, rescan in your DAW.

### Troubleshooting (macOS/Windows/Linux)

If the plugin fails to load or shows as "incompatible":
1. **macOS**: Ensure you moved the plugin to the correct system directory. If it still fails, try running `auval -v aufx MXTn Manu` in a terminal to see if the system reports any signature or dependency errors.
2. **DAW Cache**: Most DAWs (like REAPER, Ableton, or Logic) cache failed plugin scans. You must "Clear Cache and Rescan" in your DAW preferences after an update.
3. **Permissions**: On macOS, if you get a "Developer cannot be verified" message, go to **System Settings > Privacy & Security** and click "Allow Anyway" for the plugin.

## Developer Notes

This project is built using **JUCE 7** and **CMake**. It bridges legacy C-based pitch algorithms with modern C++ plugin wrappers.

### Architecture & Dependencies

The plugin relies on several high-performance audio libraries:
- **RubberBand**: Used for high-quality time-stretching and pitch-shifting.
- **SoundTouch**: Provides an alternative pitch-shifting engine.
- **Aubio**: Real-time pitch detection (YIN/FFT based).
- **FFTW3**: Handles fast Fourier transforms for spectral processing.

On macOS, these are typically installed via Homebrew for development, but the `build_macos.sh` script is designed to make the final plugin **self-contained**.

### macOS Bundling & Signing (Important)

Standard macOS plugins are often dynamically linked to Homebrew libraries, which causes them to crash on other users' machines. Our build process handles this automatically:
1. **Framework Bundling**: Copies `.dylib` files from `/opt/homebrew/` into the plugin's `Contents/Frameworks/` directory.
2. **RPath Fixing**: Uses `install_name_tool` to change the load paths from absolute paths to `@loader_path/../Frameworks/`.
3. **Deep Signing**: Since ad-hoc signing on macOS is strict, we sign the internal dylibs individually before performing a "deep" signature on the entire `.vst3` or `.component` bundle.

### JUCE & macOS 15 Compatibility

JUCE 7.0.5 has a known issue with `CGWindowListCreateImage` on macOS 15 Sequoia (where the API was deprecated/removed). The `build_macos.sh` script includes a Python-based patcher that automatically guards this call in the JUCE source code before building to ensure stability on the latest macOS versions.

### Projucer vs CMake

While the project uses a `.jucer` file to manage JUCE modules, the actual build orchestration is handled by `CMakeLists.txt`. The `build_macos.sh` script runs the Projucer CLI to regenerate the `JuceLibraryCode` before handing off the build to CMake.

## Hot keys

- `alt + mouse wheel` — x zoom
- `ctrl + mouse wheel` — y zoom
- `mouse wheel` or `W / S` — y move
- `shift + mouse wheel` or `A / D` — x move
- `left button drag` — add note
- `right button` — delete note

## Building from source

### macOS

Prerequisites: Homebrew, Xcode command-line tools, CMake, Git.

```bash
./build_macos.sh
```

The script installs dependencies, downloads JUCE 7.0.5, applies the macOS 15+ compatibility patch, clones the VST3 SDK, builds, and installs the plugin to `/Library/Audio/Plug-Ins/VST3/`.

### Linux

```bash
sudo apt install libfftw3-dev libsamplerate0-dev libaubio-dev \
  libsoundtouch-dev librubberband-dev libx11-dev libxext-dev \
  libxrandr-dev libfreetype-dev libasound2-dev cmake pkg-config
./build_linux.sh
```

### Windows (MSYS2 / MinGW-w64)

```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-fftw mingw-w64-x86_64-aubio \
  mingw-w64-x86_64-soundtouch mingw-w64-x86_64-rubberband
mkdir build-cmake && cd build-cmake
cmake .. -DCMAKE_CXX_COMPILER=g++ -DCMAKE_C_COMPILER=gcc -G "Unix Makefiles"
make -j$(nproc)
```
