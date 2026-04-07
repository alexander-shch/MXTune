# MXTune

[![Build and Release](https://github.com/alexander-shch/MXTune/actions/workflows/release.yml/badge.svg)](https://github.com/alexander-shch/MXTune/actions/workflows/release.yml)

An open-source auto-tune / pitch correction audio plugin (VST3) with real-time pitch detection and correction to a user-defined musical scale or MIDI-driven note mapping.

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
