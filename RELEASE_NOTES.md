# Wipeout2K97XL v0.1.1 — Optional widescreen HUD layout

**Steam Deck AppImage update:** use the **-r2.AppImage** download. It fixes a
reproduced startup failure where the original image could not open an OpenGL
window and misleadingly exited with “no BIOS selected.” The corrected wrapper
passes launcher tests on Ubuntu 22.04 and 24.04; physical Deck confirmation
is pending. The original AppImage is marked superseded. Other platform kits
are unchanged. [r2 verification](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/39373180675).

The 16:9 mod now includes **Race HUD layout → Widescreen edges**:

- Checkpoint timer at the top left.
- Position and lap at the top right.
- Speed, thrust, energy and music crawl at the bottom right.
- Lap-times heading, lap numbers and times at the bottom left.

The original centered HUD remains the default. Change the dropdown under
**Mods → 16:9 Widescreen**, then launch the game. HUD size and game simulation
are unchanged. Menus, pause text and the central weapon display stay centered.
The layout was tested with OpenBIOS on Windows and approved by the owner.

| Download | Platform | Status |
| --- | --- | --- |
| `wipeoutxl-0.1.1-windows-x64.zip` | Windows 10/11, Intel/AMD x64 | Windows gameplay and controllers tested |
| `wipeoutxl-0.1.1-linux-x64.zip` | Linux x64 | Native build/launcher tested; gameplay unverified |
| `wipeoutxl-0.1.1-macos-x64.zip` | macOS 15, Intel | Native build/launcher tested; gameplay unverified |
| `wipeoutxl-0.1.1-macos-arm64.zip` | macOS 15, Apple Silicon | Native build/launcher tested; gameplay unverified |
| `wipeoutxl-0.1.1-steamdeck-x86_64-r2.AppImage` | Steam Deck / Linux x64 | Packaging/launcher tested; Steam hardware and gameplay unverified |

Every download has a SHA-256 checksum. These are **setup kits**: supply your
legally obtained Wipeout XL USA SCUS-94351 CUE and all 12 BIN tracks, keep
OpenBIOS selected, then use **Generate & rebuild**. No disc, generated game code,
retail BIOS, memory cards or captures are distributed. Windows/macOS binaries
are unsigned. ZIP users need Python 3.11+; the AppImage bundles Python.

Existing users should extract into a new writable folder and regenerate/rebuild
from their own disc. Back up saves and custom settings before updating.
A new AppImage creates a new versioned writable project while preserving the
shared profile. Rebuild that version before playing with the new HUD option.

Includes the existing configurable NeGcon enhanced dual sticks, analog steering
and acceleration, Steam Input gamepad route, WASD controls, extended draw distance,
and JVC/Trinitron-inspired CRT effects. Framework and recomp-ui pins are retained.
Extended draw distance is a regular option. Visual mods remain off by default.

[Setup instructions](https://github.com/NJH-1001/Wipeout2K97XL#download-and-setup) ·
[Steam Deck guide](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/steam-deck.md) ·
[HUD verification](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/verification/hud-edges.json)


Build provenance: [Windows](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/39351929147),
[Linux and both macOS targets](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/39351930903),
and [Steam Deck AppImage](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/39352426493).
See the [release verification manifest](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/verification/release-0.1.1.json)
for artifact hashes and the Windows extracted-kit rebuild check.
