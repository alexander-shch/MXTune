# Building MXTune from Source

## macOS

Prerequisites: Homebrew, Xcode command-line tools, CMake, Git.

```bash
./build_macos.sh
```

The script handles framework bundling, rpath fixing, and deep-signing automatically. See [DEVELOPER_NOTES.md](./DEVELOPER_NOTES.md#macos-self-contained-bundles) for details.

## Windows (MSYS2 / MinGW-w64)

Prerequisites: [MSYS2](https://www.msys2.org/), MINGW64 environment.

```bash
./build_windows.sh
```

Dependencies are statically linked to avoid missing DLL issues at runtime.

## Linux

Prerequisites: GCC, CMake, pkg-config, and development headers for FFTW, SoundTouch, Aubio, RubberBand, libsamplerate.

```bash
./build_linux.sh
```

The Linux build produces a dynamically linked `.so` inside a standard `.vst3` bundle structure.
