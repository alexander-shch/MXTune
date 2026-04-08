#!/bin/bash
set -e

# This script is intended to be run in MSYS2 with the MINGW64 environment.
# It automates the installation of dependencies and the CMake build process.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

GREEN='\033[0;32m'
NC='\033[0m'
echo_step() { echo -e "${GREEN}==> $1${NC}"; }

# ---------------------------------------------------------------------------
# 1. Install Dependencies via pacman
# ---------------------------------------------------------------------------
echo_step "Installing dependencies via pacman..."
pacman -S --needed --noconfirm \
    mingw-w64-x86_64-toolchain \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-pkg-config \
    mingw-w64-x86_64-python3 \
    mingw-w64-x86_64-fftw \
    mingw-w64-x86_64-aubio \
    mingw-w64-x86_64-soundtouch \
    mingw-w64-x86_64-rubberband \
    zip unzip curl git

# ---------------------------------------------------------------------------
# 2. JUCE — download and run Projucer to regenerate JuceLibraryCode
# ---------------------------------------------------------------------------
mkdir -p "$SCRIPT_DIR/third_party"
pushd "$SCRIPT_DIR/third_party"

if [ ! -f "juce-7.0.5-windows.zip" ]; then
    echo_step "Downloading JUCE 7.0.5..."
    curl -fL "https://github.com/juce-framework/JUCE/releases/download/7.0.5/juce-7.0.5-windows.zip" \
         -o "juce-7.0.5-windows.zip"
fi

if [ ! -d "JUCE" ]; then
    echo_step "Extracting JUCE..."
    unzip -q juce-7.0.5-windows.zip
fi

PROJUCER="$(pwd)/JUCE/Projucer.exe"
echo_step "Configuring JUCE module paths..."
"$PROJUCER" --set-global-search-path windows defaultJuceModulePath "$(pwd)/JUCE/modules"
echo_step "Regenerating JuceLibraryCode..."
"$PROJUCER" --resave "$SCRIPT_DIR/JUCE/mx_tune.jucer"

popd

# ---------------------------------------------------------------------------
# 3. VST3 SDK
# ---------------------------------------------------------------------------
if [ ! -d "$SCRIPT_DIR/VST3_SDK" ]; then
    echo_step "Cloning VST3 SDK..."
    git clone --recursive https://github.com/steinbergmedia/vst3sdk.git "$SCRIPT_DIR/VST3_SDK"
fi

echo_step "Checking out VST3 SDK v3.7.1_build_50..."
pushd "$SCRIPT_DIR/VST3_SDK"
git checkout v3.7.1_build_50
git submodule update --recursive
popd

# ---------------------------------------------------------------------------
# 4. Build MXTune
# ---------------------------------------------------------------------------
echo_step "Building MXTune..."
mkdir -p "$SCRIPT_DIR/build-cmake"
pushd "$SCRIPT_DIR/build-cmake"
cmake "$SCRIPT_DIR" -G "Ninja" -DCMAKE_BUILD_TYPE=Release
ninja -j$(nproc)
popd

PLUGIN_VERSION="$(cat "$SCRIPT_DIR/VERSION" | tr -d '[:space:]')"
echo_step "Done! MXTune ${PLUGIN_VERSION} built successfully in build-cmake/"
echo_step "Windows VST3: build-cmake/mx_tune_vst3/Release/mx_tune.vst3"
