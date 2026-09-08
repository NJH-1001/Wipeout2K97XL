# OpenBIOS allocator compatibility diagnosis

Date: 2026-09-07. General OpenBIOS user allocator correction implemented and independently probe-verified; native post-intro rendering reached.

## Observed call chain

Beetle was restarted with the verified external OpenBIOS image. TCP trace ranges were armed by frame 19, before game entry. Bounded rings were captured under ignored `analysis/verification/heap-writes.json` and `heap-calls.json`.

1. At frame 609, boot code calls BIOS InitHeap with base 0x800FE100 and size 0x000F9EFC, returning to 0x80082E00. OpenBIOS writes the user-heap head at RAM 0x6544.
2. The intro overlay function at 0x8011A394 allocates its buffers through 0x8011B198, the overlay's custom allocator. Ghidra shows 16-byte headers, four-byte payload alignment, and allocated/free tags. Its matching free routine is 0x8011B3D8.
3. At frame 1961, the custom allocator writes the header for pointer 0x80195F10. The allocation is 0x4000 bytes. Ghidra and the write trace agree on the header-producing instructions, including 0x8011B298.
4. At frame 6052, cleanup calls trampoline 0x80122100 from 0x8011A954 with pointer 0x80195F10, returning to 0x8011A95C. The trampoline selects A0:34, BIOS free, rather than the custom allocator's matching free routine.
5. Both independent Beetle and the native build enter OpenBIOS's copied `multi_free` implementation and repeatedly walk the list at RAM 0x2254/0x22DC. The caller and pointer agree across both processes.

## Why the BIOS implementations differ

[PSX-SPX documents retail A0:34](https://psx-spx.consoledev.net/kernelbios/#a34h-freebuf) as an unconditional read/modify/write of the word immediately before the supplied pointer, setting bit zero. It performs no pointer validation or list walk. Thus this particular call has a documented finite operation even though the pointer came from another allocator.

[The pinned OpenBIOS implementation](https://github.com/grumpycoders/pcsx-redux/blob/55fbf0468345acfbe5512628a3be5c0cfe3f22e7/src/mips/openbios/kernel/alloc.c) uses its own header layout and traverses a marker-terminated list. It therefore interprets this input differently. Current upstream and an earlier OpenBIOS allocator revision were inspected; neither supplies a drop-in retail-compatible allocator implementation.

This identifies an OpenBIOS compatibility mechanism, not a native rendering-only failure. It does not prove that changing one free operation is sufficient for the whole title. The existing OpenBIOS malloc/realloc/InitHeap functions depend on their own layout; simply replacing free with the retail bit operation would break their ownership bookkeeping.

## Required next gate

A general allocator correction must treat malloc, free, realloc and InitHeap as a coherent subsystem. Verify allocation alignment, metadata, splitting/coalescing, exhaustion, zero-sized requests, reinitialization, alias addresses, invalid free behavior and realloc copying against an independent retail BIOS reference. Keep the production title on OpenBIOS LLE. No runtime HLE shim, forced return, alternate cleanup call, or generated-code edit is authorized as a substitute for that proof.

The user supplied `../bios/PSXONPSP660.BIN` for reference testing only. Its SHA-256 is `cbe758e1c8ece593c8e14ce1e8b3436428a01c608032a02613b3a4b442b4d712`. The oracle's complete 512 KiB ROM readback matches this file. This is the PSP-hosted PS1 BIOS reference supplied by the user; its allocator edge cases must not be assumed universal across retail PS1 BIOS revisions. Wipeout XL passed the intro cleanup and reached rendered 3D attract/credits with this reference; ignored capture `captures/reference-after-intro.bmp` records that observation. The target remains OpenBIOS.

## Independent homebrew probe

Original source in `tools/allocator_probe` exercises the A0 allocator services through a booted PS-X EXE. A local boot-area fixture extracted from the owned disc was necessary for reference BIOS disc boot. The fixture and synthetic disc stay under ignored `build-allocator-probe`; no firmware or game bytes are included in the tracked probe source.

Both reference and unmodified OpenBIOS completed the isolated 17-record sequence in independent Beetle processes. The collector verifies the entire selected ROM, then captures bounded result records and heap snapshots over TCP. Heap base is 0x80100000 and InitHeap size is 0x400.

| Observation | Supplied reference | OpenBIOS |
| --- | --- | --- |
| First malloc(1) result | 0x80100004 | 0x80100008 |
| Subsequent malloc(5) result | 0x8010000C | 0x80100018 |
| malloc(0), after reinitialization | 0x80100004 | 0x80100008 |
| malloc(0x1000), after zero allocation | 0xFFFFFFFF | 0x00000000 |
| free(0x8007FF04), preceding guard initially 0x12345678 | guard becomes 0x12345679 | guard remains 0x12345678 |

The reference free body at ROM 0xBFC020F0 agrees with the observed guard update. The OpenBIOS probe's invalid free returns under this heap state; it is the game's different heap state that causes the previously traced list-walk stall. This probe does not claim every invalid free hangs.

The initial sequence also exposed a reference realloc edge case: shrink 32 to 16, then grow to 64, leaves the next header walk out of alignment with the sentinel; the following calloc did not finish. Its bounded pre-stall snapshot is preserved as `allocator-reference-initial.bin`. The current probe explicitly reinitializes the heap before calloc, retaining both realloc snapshots. This isolation permits subsequent cases to complete; it does not declare the realloc issue resolved or discard the failure.

Remaining verification gate: a coherent general OpenBIOS allocator correction, tested against these reference vectors plus aliasing, copying, overflow and broader sequence coverage, then rebuilt/recompiled and rechecked in the native title. No allocator source replacement has been applied. Full rendering analysis, controller/mod validation and release packaging remain incomplete.

Raw traces, headers and decompiled game code stay ignored; this document contains only the diagnostic facts and addresses.

## General correction verified

The OpenBIOS source patch is `psxrecomp/bios/patches/openbios-user-heap.patch`, against pcsx-redux 55fbf0468345acfbe5512628a3be5c0cfe3f22e7 and uC-sdk 69e06871824e2d62069487a7426ded09090ceb69. It replaces the user malloc/free/realloc/InitHeap ABI together; the private kernel allocator remains separate. There are no game addresses or service interception hooks in the patch. The BIOS is rebuilt as MIPS and statically recompiled.

Expanded testing completed 32 directed records under both the supplied reference ROM and rebuilt OpenBIOS. Every result byte and every 4096-byte heap snapshot matches, including aliasing, moved realloc copying, zero heap size, wrapped allocation and calloc multiplication. The comparator rejects partial completion. Raw-result SHA256 is `f121f50cfd7bcb87f8082b61e1dd424c86c27fde910e5b481bfff225a8ecd261` for both. The unspecified return from realloc(ptr,0) is normalized by the probe; its free side effect is compared. This is directed coverage against one supplied BIOS revision, not exhaustive equivalence.

The candidate preserves the reference's observed realloc metadata defect; matching snapshots do not mean that the original failing allocator sequence becomes safe. This supersedes the earlier unimplemented-correction gate above. No per-game cleanup bypass was introduced.

Rebuilt ROM SHA256: `6bee7e0b77c41ec069122127b6ec897eb7799fdf27a0bab38fb4f42424799b3d`. Its ELF-derived relocation windows are byte-verified. Fresh ELF seeds and discovery closure emit 702 functions / 18043 instructions / 4096 dispatch entries, with zero skipped, interpreted or unsupported functions. Old-ROM empirical offsets were not blindly reused after the compiler/layout change. See `openbios-allocator-build.json`.

Windows RelWithDebInfo rebuild succeeded. Native OpenBIOS now renders the 3D credits/attract scene beyond the former cleanup stall; independent Beetle with the same OpenBIOS reaches the title and a 3D demo race. A later native frame showed corruption, followed by clean 3D frames; matched-state analysis is still needed to identify its cause. Controllable native racing, complete rendering equivalence and packaging are not yet verified.
