# Linux and macOS setup kits

Native setup kits are built on Linux x64, Intel Mac, and Apple Silicon Mac.
They use the same game configuration, OpenBIOS and controller mod as Windows.
They contain no game data. Supply your own USA SCUS-94351 disc locally.

## Verification status

[Native CI](https://github.com/NJH-1001/Wipeout2K97XL/actions/workflows/native-release.yml)
builds both emitters and a RelWithDebInfo setup launcher on each native OS.
The release gate checks archive contents, CPU architecture, executable permissions,
dynamic dependencies and emitter startup. It then extracts the kit, opens the
launcher and captures its visible first-run window.

These checks do **not** establish successful local game generation, compilation,
OpenBIOS game boot, a completed race, controller behavior or display-mod behavior
on Linux/macOS. No Linux or Mac test machine with the owner's disc is available.
These kits are published as **experimental previews — gameplay unverified**,
with the project owner’s explicit approval.
[Download the previews](https://github.com/NJH-1001/Wipeout2K97XL/releases/tag/v0.1.0-native-preview).
Windows remains the gameplay-tested release. CI never receives the disc or generated game code.

## Targets and prerequisites

| Kit | Target |
| --- | --- |
| `linux-x64` | Linux x86-64, built on Ubuntu 22.04 (glibc 2.35) |
| `macos-x64` | macOS 15, Intel x86-64 |
| `macos-arm64` | macOS 15, Apple Silicon arm64 |

Use Python 3.11 or newer. SDL3 is linked statically into the launcher. Linux
still requires the system C/C++ runtime, zlib, OpenGL and a working graphical
desktop/GPU driver. The Mac binaries link only Apple system libraries/frameworks;
Homebrew libraries are not required to start the launcher. Mac binaries are not
Developer ID signed or notarized.

Extract the complete ZIP into a writable directory. From that directory run:

```sh
chmod +x Wipeout2K97XL psxrecomp/recompiler/build/psxrecomp-game psxrecomp/recompiler/build/psxrecomp-bios
./Wipeout2K97XL
```

The wizard offers the portable toolchain or an existing system CMake/compiler.
System builds need CMake, Ninja, a C/C++ compiler and development dependencies.
On macOS, install Xcode Command Line Tools. On Linux, see SDL's
[build dependency list](https://wiki.libsdl.org/SDL3/README-linux).
The supplied workflow records the exact build dependencies used in CI.

Before promoting these kits, test a fresh extracted kit end to end: select the
owned disc, generate and rebuild, boot with OpenBIOS, complete a race, check the
default dual-stick controls, save/reload and test enabled display mods. Record
OS, architecture, GPU, controller model and any failures.
