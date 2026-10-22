# Wipeout2K97XL v0.2.0 - Smooth rendering

Optional **Smooth rendering** adds 60, 90 and 120 fps choices under **Mods >
Visual > Render frame rate**, including the main menu's 3D previews. It renders
intermediate polygon positions while preserving the original simulation,
steering, physics, timers and audio cadence. OpenGL is required. The feature
is off by default. HUD and effects retain their original update rate.

This update also corrects shared-vertex fallback that could separate road
polygons, retains polygons that enter view in an intermediate frame, and
refreshes cached menu backgrounds to prevent preview trails.

Interpolation adds visual delay. Higher rates require additional GPU work and
a suitable display. Physical input-to-photon latency and exhaustive track,
camera and effect coverage remain unverified. The owner accepted the Windows
racing appearance; native gameplay verification does not transfer to this build.

| Download | Platform |
| --- | --- |
| `wipeoutxl-0.2.0-windows-x64.zip` | Windows 10/11 x64 |
| `wipeoutxl-0.2.0-linux-x64.zip` | Linux x64, Ubuntu 22.04 baseline |
| `wipeoutxl-0.2.0-macos-x64.zip` | macOS 15 Intel |
| `wipeoutxl-0.2.0-macos-arm64.zip` | macOS 15 Apple Silicon |

These are setup kits. Supply your legally owned USA SCUS-94351 CUE and all
12 BIN tracks, keep OpenBIOS selected, then **Generate & rebuild**. Existing
users should extract to a new writable folder, regenerate and rebuild, and
back up saves and custom settings. No disc, generated game code, retail BIOS,
memory cards or captures are distributed. Windows/macOS binaries are unsigned.
Python 3.11+ is required. Every archive has a SHA-256 checksum.

The Linux ZIP from the prior release was owner-tested on Steam Deck; this
version's smoothing is not physically tested there. macOS gameplay remains
unverified. Dedicated AppImages remain withdrawn.

Includes the existing configurable NeGcon dual sticks, WASD, widescreen HUD
layout, extended draw distance and CRT effects. Framework/UI pins are retained.

[Setup instructions](https://github.com/NJH-1001/Wipeout2K97XL#download-and-setup) ·
[Rendering details](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/framerate.md)
