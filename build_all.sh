#!/bin/bash

# ==============================================================================
# Godot C++ Module Multi-Target Cross-Compilation Script
# Designed for use in WSL (Ubuntu/Linux) to build Windows, Linux, and macOS binaries.
#
# Execution Order: Windows Debug -> Windows Release -> Linux Debug -> Linux Release
#                  -> macOS Debug -> macOS Release (if OSXCross available)
#
# NOTES:
# - Windows builds require MinGW-w64: 'sudo apt install mingw-w64'
# - macOS builds require OSXCross toolchain (optional, will skip if not found)
#
# To skip macOS builds, run: ./build_all.sh --skip-macos
# ==============================================================================

# Parse command line arguments
SKIP_MACOS=false
for arg in "$@"; do
    case $arg in
        --skip-macos)
            SKIP_MACOS=true
            shift
            ;;
    esac
done

# Determine the number of available CPU cores for parallel compilation (speed boost)
CORES=$(nproc)

# Check if OSXCross is available
OSXCROSS_AVAILABLE=false
if [ -z "$OSXCROSS_ROOT" ]; then
    export OSXCROSS_ROOT="/opt/osxcross"
fi

if [ -f "$OSXCROSS_ROOT/target/bin/x86_64-apple-darwin23-clang++" ] || \
   [ -f "$OSXCROSS_ROOT/target/bin/arm64-apple-darwin23-clang++" ]; then
    OSXCROSS_AVAILABLE=true
fi

# Determine build count
BUILD_COUNT=4
if [ "$OSXCROSS_AVAILABLE" = true ] && [ "$SKIP_MACOS" = false ]; then
    BUILD_COUNT=6
fi

echo "Starting Godot C++ Module Sequential Compilation..."
echo "Compiling a matrix of ${BUILD_COUNT} artifacts using ${CORES} concurrent jobs for each step."
if [ "$OSXCROSS_AVAILABLE" = false ] && [ "$SKIP_MACOS" = false ]; then
    echo "Note: OSXCross not found, skipping macOS builds"
elif [ "$SKIP_MACOS" = true ]; then
    echo "Note: macOS builds skipped (--skip-macos flag)"
fi
echo "----------------------------------------------------------------------"

# --- 1. WINDOWS DEBUG BUILD ---
# Builds shared library (.dll) for Windows, optimized for debugging.
echo ""
echo "-> 1/${BUILD_COUNT}: Building windows | Target: template_debug (using MinGW cross-compiler)"
scons platform=windows target=template_debug use_mingw=yes -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Windows Debug build. Review output and correct errors."
    exit 1
fi

# --- 2. WINDOWS RELEASE BUILD ---
# Builds shared library (.dll) for Windows, optimized for production release.
echo ""
echo "-> 2/${BUILD_COUNT}: Building windows | Target: template_release (using MinGW cross-compiler)"
scons platform=windows target=template_release use_mingw=yes -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Windows Release build. Review output and correct errors."
    exit 1
fi

# --- 3. LINUX DEBUG BUILD ---
# Builds shared object (.so) for Linux desktop, optimized for debugging.
# NOTE: The platform flag 'linux' is used as requested. If compilation fails,
# you may need to use 'linuxbsd' (for Godot 4.x) or 'x11' (for Godot 3.x) instead.
echo ""
echo "-> 3/${BUILD_COUNT}: Building linux | Target: template_debug (using native GCC/Clang)"
scons platform=linux target=template_debug -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Linux Debug build. Review output and correct errors."
    exit 1
fi

# --- 4. LINUX RELEASE BUILD ---
# Builds shared object (.so) for Linux desktop, optimized for production release.
echo ""
echo "-> 4/${BUILD_COUNT}: Building linux | Target: template_release (using native GCC/Clang)"
scons platform=linux target=template_release -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Linux Release build. Review output and correct errors."
    exit 1
fi

# --- 5. MACOS DEBUG BUILD (OPTIONAL) ---
# Builds dynamic library (.dylib) for macOS, optimized for debugging.
# NOTE: Only runs if OSXCross toolchain is available.
if [ "$OSXCROSS_AVAILABLE" = true ] && [ "$SKIP_MACOS" = false ]; then
    echo ""
    echo "-> 5/${BUILD_COUNT}: Building macos | Target: template_debug (using OSXCross)"

    scons platform=macos target=template_debug osxcross_sdk=darwin23 -j"${CORES}"

    if [ $? -ne 0 ]; then
        echo "!! Error during macOS Debug build. Review output and correct errors."
        exit 1
    fi

    # --- 6. MACOS RELEASE BUILD ---
    # Builds dynamic library (.dylib) for macOS, optimized for production release.
    echo ""
    echo "-> 6/${BUILD_COUNT}: Building macos | Target: template_release (using OSXCross)"
    scons platform=macos target=template_release osxcross_sdk=darwin23 -j"${CORES}"

    if [ $? -ne 0 ]; then
        echo "!! Error during macOS Release build. Review output and correct errors."
        exit 1
    fi
fi

echo "----------------------------------------------------------------------"
if [ "$BUILD_COUNT" -eq 6 ]; then
    echo "All six artifacts compiled successfully in the requested order."
    echo "The resulting binaries (.so, .dll, and .dylib) are located in the 'bin/' subdirectory."
else
    echo "All four artifacts compiled successfully in the requested order."
    echo "The resulting binaries (.so and .dll) are located in the 'bin/' subdirectory."
    if [ "$OSXCROSS_AVAILABLE" = false ]; then
        echo ""
        echo "Note: To build for macOS, install OSXCross. See setup instructions in script comments."
    fi
fi