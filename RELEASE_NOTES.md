# Wipeout2K97XL v0.1.0 — Windows, Linux and macOS setup kits

All platform downloads are together on this release page:

| Setup ZIP | Platform | Verification |
| --- | --- | --- |
| `wipeoutxl-0.1.0-windows-x64.zip` | Windows 10/11, Intel/AMD x64 | User-tested racing and controllers |
| `wipeoutxl-0.1.0-linux-x64.zip` | Linux x64, built on Ubuntu 22.04 | Build/launcher tested; gameplay unverified |
| `wipeoutxl-0.1.0-macos-x64.zip` | macOS 15, Intel | Build/launcher tested; gameplay unverified |
| `wipeoutxl-0.1.0-macos-arm64.zip` | macOS 15, Apple Silicon | Build/launcher tested; gameplay unverified |

Each ZIP has a SHA-256 checksum. Supply your legally obtained Wipeout XL USA
SCUS-94351 CUE and all 12 BIN tracks. No game data or retail BIOS is included.
Python 3.11+ is required; keep OpenBIOS selected and use Generate & rebuild.
Windows users run `Wipeout2K97XL.exe`; Linux/macOS users run `./Wipeout2K97XL`.
See [setup instructions](https://github.com/NJH-1001/Wipeout2K97XL#download-and-setup)
and [Linux/macOS prerequisites](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/native-platforms.md).

Includes configurable **NeGcon enhanced dual sticks**, analog steering and
trigger acceleration, WASD keyboard controls, optional 16:9 presentation,
extended scenery/ship/exhaust distance, and JVC/Trinitron-inspired CRT effects.
Visual options remain off by default. Windows and Mac binaries are unsigned.

## 2026-10-19 maintenance refresh

- Extended Draw Distance and Extended Enemy Ship Distance are regular optional
  features following the owner's race testing. The launcher labels and descriptions
  no longer call them experiments. Saved feature identifiers and behavior are unchanged.
- Linux and both Mac builds are now alongside Windows, with their existing
  **gameplay unverified** status retained.
- Every kit includes current instructions, the final controller diagram and the
  [dependency review](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/dependency-review-2026-10-19.md).
- Build workflows use current official GitHub actions pinned to immutable commits.

This is a documentation and declarative-catalog refresh of the audited kits.
Runtime binaries, emitters, firmware and game/plugin sources remain unchanged.
Checksums therefore differ from the earlier ZIPs. No runtime upgrade is implied.

Binary provenance: [Windows CI 34230697278](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/34230697278),
source `160a562601b96ba9b65bc76d2f5ab4dcd987e552`, and
[native CI 34263238594](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/34263238594),
source `49c024a373e7bee20aa32df5caf60b1d9b8a3511`.
See the [maintenance manifest](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/verification/maintenance-2026-10-19.json)
for archive identities and exact metadata changes. Keep saves and custom settings backed up.

## Steam Deck AppImage preview

`wipeoutxl-0.1.0-steamdeck-x86_64.AppImage` and its checksum are now available
on this page. It bundles Python and creates a writable project folder for local
generation/rebuilding, preserving shared saves and settings across image updates.
Use Steam Input with a Gamepad layout for Steam Deck controls and the wireless
Steam Controller (2026). **Hardware, Gaming Mode and gameplay remain unverified.**
See the [Steam setup guide](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/steam-deck.md)
and [AppImage verification](https://github.com/NJH-1001/Wipeout2K97XL/blob/main/docs/verification/appimage.json).
The setup launcher and emitters remain identical to the released Linux kit.
