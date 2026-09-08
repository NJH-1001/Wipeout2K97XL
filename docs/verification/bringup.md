# Wipeout XL bring-up verification — 2026-09-07

Status: **BLOCKED at intro cleanup with OpenBIOS in both native and independent Beetle.** A local Windows x86-64 RelWithDebInfo executable exists and reaches the animated game intro. Gameplay and a distributable release are not verified.

## Proven locally

- Ghidra MCP connected to project Wipeout2K97XL on localhost:8089 (238 analysis tools registered).
- PS-X EXE import was not recognized. Imported its header-free payload using MIPS:LE:32:default, rebased to header load address 0x80010000, and created BootEntry at 0x80082D70.
- Ghidra entry-point bytes match the extracted executable exactly: 0980023c5c4942241080033cfce06324000040ac040042242b084300fcff2014.
- Framework probe returned no warnings, serial SCUS-94351, boot SCUS_943.51, volume WIPEOUT2, 12 tracks (one data, eleven audio), load 0x80010000, entry 0x80082D70, text size 0x85000, stack 0x801FFFF0.
- Track 01: 86045568 bytes; MD5 bee4ab5e4835235ccf6f16bc8caea5e6; SHA1 5694008c30fa9896d17afdd6281e4bebb9f63953; CRC32 18ebdeb3. These identify the supplied input; external Redump hash matching is NOT established.
- Boot EXE SHA256: 359270b4a0e3f41a55d07385d7ea37ec978a38f3270ddab3dae3e4057d035b04.
- Initial seeds: 755 entry/direct-JAL targets from the framework probe, not a fully verified Ghidra function inventory.
- Framework 8d56cf659fd84367c3f778e17851674ac7896371; launcher 773155ae7d3be80a21d40851b58f99c79e003de1. Separate local title repository created; no remote publishing.
- Both emitter build targets compiled successfully with MinGW GCC 16.2.0, RelWithDebInfo.
- Generate returned success and emitted 1423 game functions into 40 C shards. Emission is not runtime verification.

## Resolved generation and build blockers

The four former skipped OpenBIOS candidates were data, not missing instruction implementations. Ghidra confirmed an unconditional branch before 0xBFC095F8 and the other three candidates inside the copied A0 function-pointer table. The fastMemset computed branch used a masked index; the emitter had selected an unrelated comparison as its table bound.

Changed the discovery/emitter and ELF seed tool, removed the four demonstrated data seeds, and regenerated. The emitter now writes discovery proof before emission and fails on unsuccessful discovery. Empty skipped/interpreted reports overwrite stale reports from prior runs.

Current result: **643 initial BIOS seeds; 695 emitted functions; 0 interpreted; 0 skipped; 17,522 instructions; 3,984 dispatch entries.** `unsupported_ops.json` is empty. `openbios-skipped-functions.json` in this documentation directory is historical evidence of the original failure, not current output. Current generator reports live under ignored `psxrecomp/generated/`.

The netplay-disabled launcher build referenced unavailable lobby fields. Added compile-time guards around lobby-only code in framework source. No generated C was edited.

`build-win64/Wipeout2K97XL.exe` builds with Clang 22.1.8, SDL3, RelWithDebInfo, Windows x86-64. It links the generated OpenBIOS backend and runs with `bios_hle=false` / `PSX_BIOS_HLE=0`. This is a local game-code-bearing build, not a redistributable package.

## Independent oracle restored and exercised

Recovered and reviewed ten observer-only Beetle source changes against upstream commit `5759277be50052b9f3f388578bf56cc7899d833f`. The complete patch is preserved in `psxrecomp/docs/beetle-observers.patch`; the dependency checkout is ignored. Built the core and `build-oracle/psx-beetle.exe` from source.

Replaced the framework's empty cycle-watch and exception-ring implementations with bounded real observations. Connected the SPU observer. Cycle-watch tests cover physical-address aliases, absolute samples, A-to-B elapsed cycles, repeated anchors, clearing, default counts and capacity limits. Live TCP returned three increasing cycle samples and a populated 1,024-entry exception ring. No fake empty success is used as evidence.

Fixed explicit external OpenBIOS selection: the harness previously selected only the BIOS directory and could use the core's bundled fallback. Read the entire running oracle ROM through TCP and compared it with the requested file: **524,288 bytes, exact match**, SHA256 `fabe498fbf224e4721f12f31b6f5fe0659205e341dc4e5c5f91b9bd1a1011c57`.

Ports: native 4370, independent Beetle 4380. Raw bounded evidence and screenshots remain ignored under `analysis/verification/` and `captures/`. Screenshots visibly show game loading/intro output. Neither successful compilation nor matching failure is treated as proof of correct gameplay.

## Historical verification blocker (resolved below)

Native runs past the animated Designers Republic intro and stays on black 24-bit output. Beetle using the exact same external OpenBIOS also reaches the intro and then the same cleanup stall. This is not established to be a native-only rendering bug.

The runtime-loaded overlay calls `0x80122100` from `0x8011A954`, return address `0x8011A95C`. Ghidra identifies a BIOS A0 trampoline with selector `0x34` (`free`). Both processes repeatedly execute the copied OpenBIOS allocator loop around RAM `0x2254`/`0x22DC`, freeing pointer `0x80195F10`. Both have the same heap-control words at `0x6538..0x6557` and identical instructions at `0x2220..0x230F`; payload bytes elsewhere need not match across asynchronous samples.

The loop is in `multi_free` from the [pinned OpenBIOS allocator source](https://github.com/grumpycoders/pcsx-redux/blob/55fbf0468345acfbe5512628a3be5c0cfe3f22e7/src/mips/openbios/kernel/alloc.c). That implementation walks a free list terminated by a marker. The captured state reaches a null link instead of that marker. **The producer of the invalid list is not identified.** Possibilities cannot be settled by the two implementations hanging alike; no allocator or game bypass has been added.

Next verification work must trace heap initialization, allocation/free calls and the first write that violates the list invariant from boot in Beetle, then match the native path with Ghidra. Distinguish game expectations of retail heap layout, OpenBIOS behavior, and hardware/core effects before proposing a general fix. User requested stopping on verification blockers; enhancement and release work is stopped at this gate.

## Validation

- Focused recompiler tests: `full_function_emitter_test` and `bios_seed_objects_test` pass after final regeneration.
- `beetle_cycle_watch_test` passes through CTest; live instruction/exception sampling also demonstrated.
- Native and oracle executables build; OpenBIOS generation has no skipped/interpreted functions.
- Live oracle BIOS byte identity verified; visible loading/intro screenshots inspected.
- Gameplay, racing, saves, controller operation and native/oracle rendering parity have NOT passed.

## Remaining milestones and rendering scope

Full rendering analysis is explicitly requested and remains unfinished. See [rendering-pipeline.md](../rendering-pipeline.md) and [rendering-manifest.json](rendering-manifest.json) for the static inventory, discovered addresses, evidence limits, and required coverage. No claim that an instruction inventory constitutes the entire pipeline is made.

Attached Switch 2 controller verification; Switch/Switch 2/PS4/PS5/Xbox support matrix; NeGcon analog steering; WASD; CRT JVC/Trinitron options; 16:9; Windows package: pending the verified baseline. SDL3 is built in; that alone does not prove any physical controller works. `default_mode="digital"`, `lock_mode=true`, `allow_hybrid=false` remain the verification baseline.

Disc images, extracted executable, generated game C, Ghidra database, memory cards, captures and builds are local-only and ignored. A public release must follow the setup-host model so recipients generate game code from their own disc. No disc, generated game code or captures were committed or published.

## Continuation: allocator mismatch identified

The pointer passed to BIOS free is now traced to the intro overlay's custom allocator: allocation frame 1961, cleanup frame 6052. Its 16-byte header layout differs from OpenBIOS's layout. Retail BIOS free has documented mark-only behavior; OpenBIOS instead traverses its own list and hangs. See [heap-compatibility.md](heap-compatibility.md) for the call chain, primary-source comparison and remaining verification gate. This supersedes the earlier statement that the pointer producer was unknown; the complete replacement allocator is still unverified. A legally dumped retail BIOS has been requested solely as an independent test reference; the target remains OpenBIOS.

## Supplied BIOS reference and allocator probe

The supplied PSP PS1 BIOS now provides an independent comparison: exact ROM readback verified; Wipeout XL reaches 3D attract/credits after the intro. The original allocator probe completes 17 isolated records under this reference and stock OpenBIOS, confirming incompatible allocation headers and free behavior. Full findings and unresolved realloc/coverage requirements are in [heap-compatibility.md](heap-compatibility.md). OpenBIOS gameplay and release remain blocked pending a general verified allocator correction.


## Current result: OpenBIOS game boot and controller verified

The general user-allocator correction now passes all 32 directed probe records
byte-for-byte against the supplied independent reference. The rebuilt OpenBIOS
ROM and all relocation windows are recorded in openbios-allocator-build.json.
Fresh generation emits 702 functions, no skipped/interpreted/unsupported
instructions. Native displays post-intro game output; same-ROM Beetle displays
the title and demo race. The user confirmed working keyboard input, then
confirmed the attached PowerA Switch 2 gamepad after P1 auto routing was set.
The rebuilt host_pad_status command reports slot 0 kind 2, opened true, and
the exact attached GUID. See ../controllers.md for limits and defaults.

This supersedes the allocator blocker and earlier controller/WASD pending
statements. Complete rendering analysis, matched native/Beetle frame evidence,
NeGcon analog steering, other physical controller families, CRT and widescreen
mods, save verification and release packaging remain unfinished. The earlier
corrupt native transition capture still requires matched-state diagnosis.
