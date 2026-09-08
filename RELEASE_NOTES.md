# Wipeout2K97XL v0.1.0 — Windows x64 setup kit

Initial public preview for Wipeout XL (USA), SCUS-94351.

Download and extract `wipeoutxl-0.1.0-windows-x64.zip`, then run
`Wipeout2K97XL.exe`. Supply your legally obtained CUE and all 12 BIN tracks.
Keep OpenBIOS selected and use Generate & rebuild. Python 3.11+ and the
supported toolchain are required; the wizard can obtain the toolchain.

Includes configurable **NeGcon enhanced dual sticks**, analog steering and
trigger acceleration, WASD keyboard controls, optional 16:9 presentation,
extended scenery/ship/exhaust distance, and JVC/Trinitron-inspired CRT effects.
Visual mods are off by default. Windows binaries are unsigned.

The archive contains setup tools and open-source firmware, not game content.
Game code is generated locally. Keep local saves and custom settings backed up.
See README.md and VERIFICATION.md for instructions and testing limits.

The downloadable kit comes from [Windows CI run 34230697278](https://github.com/NJH-1001/Wipeout2K97XL/actions/runs/34230697278), source commit `160a562601b96ba9b65bc76d2f5ab4dcd987e552`, with runtime license notices added. Dependency audit and clean-PATH launcher startup pass.
