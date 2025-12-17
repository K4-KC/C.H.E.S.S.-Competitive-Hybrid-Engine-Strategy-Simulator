#!/bin/bash

# ==============================================================================
# Godot C++ Module Multi-Target Cross-Compilation Script
# Designed for use in WSL (Ubuntu/Linux) to build Windows and Linux binaries.
#
# Execution Order: Windows Debug -> Windows Release -> Linux Debug -> Linux Release
#
# NOTE: This script assumes you have the MinGW-w64 cross-compiler installed
# in your WSL environment (e.g., 'sudo apt install mingw-w64').
# ==============================================================================

# Determine the number of available CPU cores for parallel compilation (speed boost)
CORES=$(nproc)

echo "Starting Godot C++ Module Sequential Compilation..."
echo "Compiling a matrix of 4 artifacts using ${CORES} concurrent jobs for each step."
echo "----------------------------------------------------------------------"

# --- 1. WINDOWS DEBUG BUILD ---
# Builds shared library (.dll) for Windows, optimized for debugging.
echo "-> 1/4: Building windows | Target: template_debug (using MinGW cross-compiler)"
scons platform=windows target=template_debug use_mingw=yes -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Windows Debug build. Review output and correct errors."
    exit 1
fi

# --- 2. WINDOWS RELEASE BUILD ---
# Builds shared library (.dll) for Windows, optimized for production release.
echo "-> 2/4: Building windows | Target: template_release (using MinGW cross-compiler)"
scons platform=windows target=template_release use_mingw=yes -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Windows Release build. Review output and correct errors."
    exit 1
fi

# --- 3. LINUX DEBUG BUILD ---
# Builds shared object (.so) for Linux desktop, optimized for debugging.
# NOTE: The platform flag 'linux' is used as requested. If compilation fails, 
# you may need to use 'linuxbsd' (for Godot 4.x) or 'x11' (for Godot 3.x) instead.
echo "-> 3/4: Building linux | Target: template_debug (using native GCC/Clang)"
scons platform=linux target=template_debug -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Linux Debug build. Review output and correct errors."
    exit 1
fi

# --- 4. LINUX RELEASE BUILD ---
# Builds shared object (.so) for Linux desktop, optimized for production release.
echo "-> 4/4: Building linux | Target: template_release (using native GCC/Clang)"
scons platform=linux target=template_release -j"${CORES}"

if [ $? -ne 0 ]; then
    echo "!! Error during Linux Release build. Review output and correct errors."
    exit 1
fi

echo "----------------------------------------------------------------------"
echo "All four artifacts compiled successfully in the requested order."
echo "The resulting binaries (.so and.dll) are located in the 'bin/' subdirectory."