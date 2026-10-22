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

**Prior release Steam Deck tested:** the project owner tested the previous Linux
x64 build on Steam Deck. The new smoothing remains unverified on physical Deck. Use the standard Linux ZIP from the
[main release](https://github.com/NJH-1001/Wipeout2K97XL/releases/latest).

This is hands-on owner testing, separate from the CI build/launcher checks.
Other Linux configurations and macOS gameplay remain unverified; detailed
controller, save/reload and display-mod coverage has not been recorded.
Windows also has owner-tested gameplay and controllers. CI never receives the
disc or generated game code.

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

## Steam Deck

Use the **Linux x64 ZIP** from the main release. The project owner has tested
the prior release on Steam Deck. New rendering changes require fresh testing. Dedicated Steam Deck AppImages have been withdrawn.
See the [Steam Deck / Steam Controller guide](steam-deck.md).
