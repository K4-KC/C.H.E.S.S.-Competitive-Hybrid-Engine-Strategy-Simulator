# OSXCross Setup Guide for macOS Cross-Compilation

## What is OSXCross?

OSXCross is a cross-compilation toolchain that allows you to build macOS binaries (executables, dynamic libraries, frameworks) from Linux or other non-macOS systems. It packages Apple's Clang compiler, linker, and essential development tools to create native macOS applications without requiring a Mac.

## Why is OSXCross Needed?

Normally, building software for macOS requires:

- A physical Mac computer or macOS virtual machine
- Xcode and Apple's development tools
- macOS-specific compilers and SDKs

**OSXCross solves this problem by:**

1. **Enabling cross-platform development** - Build macOS binaries from your Linux/WSL environment alongside Windows and Linux builds
2. **Eliminating hardware requirements** - No need to purchase or maintain Mac hardware
3. **Streamlining CI/CD pipelines** - Build for all three platforms (Windows, Linux, macOS) from a single build server
4. **Supporting multi-target workflows** - Create universal binaries that work on both Intel Macs and Apple Silicon from the same environment

For this project, OSXCross allows the Godot C++ modules to be compiled into `.dylib` (dynamic library) files that can be loaded by Godot running on macOS, all while developing on Windows/WSL.

## How OSXCross Works

OSXCross uses:

- **Apple's official SDK** - Contains headers, libraries, and frameworks needed for macOS development
- **Clang/LLVM compiler** - Apple's compiler adapted for cross-compilation
- **Linker and tools** - macOS-specific build tools (ld, ar, ranlib, etc.) wrapped for Linux

The result is a complete toolchain that produces genuine macOS binaries indistinguishable from those built on a Mac.

---

## Installation Guide

This guide will help you install OSXCross on WSL/Ubuntu to enable cross-compilation for macOS targets.

## Prerequisites

Install the required packages:

```bash
sudo apt-get update
sudo apt-get install -y \
    clang \
    cmake \
    git \
    patch \
    libxml2-dev \
    libssl-dev \
    fuse \
    libfuse-dev \
    automake \
    libtool \
    pkg-config \
    llvm-dev \
    uuid-dev \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    libbz2-dev \
    zlib1g-dev \
    make \
    bash \
    tar \
    xz-utils \
    bzip2 \
    gzip \
    sed \
    cpio \
    liblzma-dev
```

## Step 1: Clone OSXCross

```bash
cd /opt
sudo git clone https://github.com/tpoechtrager/osxcross
sudo chmod -R 755 /opt/osxcross
cd osxcross
```

## Step 2: Obtain macOS SDK

You need a macOS SDK tarball. There are several options:

### Option A: If you have access to a Mac

1. Download Xcode from the Mac App Store or Apple Developer website
2. Extract the SDK using OSXCross's packaging script:

```bash
# On the Mac:
git clone https://github.com/tpoechtrager/osxcross
cd osxcross
./tools/gen_sdk_package.sh

# This creates a tarball like MacOSX14.0.sdk.tar.xz
# Transfer this file to your WSL machine
```

### Option B: Download pre-packaged SDK

Search for "MacOSX SDK" on GitHub. Several repositories host pre-packaged SDKs:
- Look for repositories like `joseluisq/macosx-sdks` or similar
- Download a recent SDK version (13.x or 14.x recommended)
- The file should be named like `MacOSX13.3.sdk.tar.xz` or `MacOSX14.0.sdk.tar.xz`

### Step 3: Place SDK in OSXCross

```bash
# Place the SDK tarball in the tarballs directory
sudo cp /path/to/MacOSX*.sdk.tar.xz /opt/osxcross/tarballs/

# Or if downloading directly:
cd /opt/osxcross/tarballs
sudo wget https://github.com/[repository]/releases/download/[version]/MacOSX14.0.sdk.tar.xz
```

## Step 4: Build OSXCross

```bash
cd /opt/osxcross
sudo UNATTENDED=1 ./build.sh
```

This will take 10-30 minutes depending on your system. The build will:
- Compile the cross-compiler toolchain
- Set up clang/clang++ for macOS targets
- Install necessary libraries and tools

## Step 5: Configure Environment Variables

Add these lines to your `~/.bashrc` or `~/.zshrc`:

```bash
# OSXCross configuration
export OSXCROSS_ROOT="/opt/osxcross"
export PATH="$OSXCROSS_ROOT/target/bin:$PATH"
```

Then reload your shell configuration:

```bash
source ~/.bashrc
# or
source ~/.zshrc
```

## Step 6: Verify Installation

Check that the compilers are available:

```bash
which x86_64-apple-darwin23-clang++
which arm64-apple-darwin23-clang++

# Test compilation
x86_64-apple-darwin23-clang++ --version
```

You should see output showing the clang version and target architecture.

## SDK Version Notes

The SDK version determines which macOS APIs are available:
- `darwin21` = macOS 12 (Monterey)
- `darwin22` = macOS 13 (Ventura)
- `darwin23` = macOS 14 (Sonoma)
- `darwin24` = macOS 15 (Sequoia)

The `build_all.sh` script uses `darwin23` by default. If you use a different SDK version, update the `osxcross_sdk` parameter in the script.

## Troubleshooting

### Permission Issues
```bash
sudo chown -R $USER:$USER /opt/osxcross
```

### Missing Libraries
If you get errors about missing libraries during build:
```bash
sudo apt-get install -y libgmp-dev libmpfr-dev libmpc-dev
```

### Compiler Not Found
Make sure the PATH is set correctly:
```bash
echo $OSXCROSS_ROOT
echo $PATH | grep osxcross
```

## Building for Different Architectures

OSXCross supports both Intel (x86_64) and Apple Silicon (arm64):

- **Intel Macs**: Use `arch=x86_64` in scons
- **Apple Silicon**: Use `arch=arm64` in scons
- **Universal binaries**: Use `arch=universal` (default in Godot)

The default Godot build creates universal binaries that work on both architectures.

## Testing Your Build

After setup, run the build script:

```bash
cd /path/to/chess-ai
./build_all.sh
```

If OSXCross is properly installed, it will build all 6 targets including macOS.

## Alternative: Skip macOS Builds

If you don't need macOS support immediately, you can skip these builds:

```bash
./build_all.sh --skip-macos
```

The script will automatically skip macOS builds if OSXCross is not detected.
