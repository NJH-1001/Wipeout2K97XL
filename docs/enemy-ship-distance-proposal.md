# Enemy ship distance proposal

Status: first candidate failed user visibility testing. A verified section-cutoff
change also remains insufficient to match the extended track range. Full far
ship projection and shared scenery ordering are the next verification gate.

The original dispatcher compares its scaled distance metric with 1200 at both
80021D1C and 80021D28 (SLTI 2A0204B0). Proposed optional range is 2900, matching
the existing broader visibility threshold. Mode, section, hidden-ship and
positive-depth checks must remain unchanged. The SDK square-root table and
far depth bounds require further verification before applying this range.

Live native and Beetle reads found the same fifteen model entries, with at most
113 primitives and maximum absolute vertex coordinate 425. GT4 output needs
52 bytes. A conservative limit of 128 primitives for each of 15 ships needs
99840 additional bytes; with the original 44032-byte frame pool that is 143872
bytes. Proposed replacement frame pool: 196608 bytes, double buffered.
The existing road/scenery DMA arena is 983040 bytes. Combined requirement is
1376256 bytes, exceeding the current 1048576-byte opt-in arena.

Proposed runtime change: enlarge the opt-in DMA-addressable memory window from
physical F00000..FFFFFF to E00000..FFFFFF (2 MiB), mapped only after allocation.
Required checks: unallocated retail DMA folding remains identical; CPU widths,
24-bit tags, allocation bounds, buffer swaps and exhaustion work correctly;
no overlap with main RAM, hardware MMIO or active guest data; serialization
and restore behavior remain explicit. Do not relax the existing LLE restore gate.

Proposed game feature: optional, guarded by exact original instruction bytes and
verified model sizes. Use supported mod APIs and regenerate any added hooks;
never edit generated code. Install a larger pool at the verified frame boundary,
preserve already-linked packets, and expose capacity/state through TCP-readable
counters. Enable a candidate only after bounds and oracle checks pass.

Risk: an incorrect memory mapping, primitive pointer or swap could corrupt GPU
packets or crash. No activation, release packaging or success claim is authorized
by this proposal alone; implementation must pass the project's verification gates.

All verification processes closed during analysis. Fresh OpenBIOS/native and
Beetle runs are required. Raw evidence is kept under ignored analysis/verification.


## Implemented candidate

`src/ship_distance_mod.c` adds the default-off `ships` feature. Its entry hook at
80021B90 validates the fifteen model headers, vertex bounds, primitive record
sizes and indices. It redirects the double-buffer pool, reserving the original
used prefix so already-linked packets remain valid in original RAM. The game's
existing frame reset subsequently selects the expanded buffer. It changes only
SLTI immediates at the two verified sites using `psx_mod_write_code_word`.
This supported API intentionally routes patched executable RAM through the
runtime's dynamic-code backend; the original ship renderer remains compiled.
Disabled diagnostic state restores both stock words. Unknown instructions or
unsupported models/pointers prevent applying the extension.

Telemetry begins at expansion address 9F000080 in this combined test build:
words 0 magic (534C5857), 1 version, 2 calls, 3 status, 4 arena, 5 per-buffer
capacity, 6 diagnostic enable, 7 patched, 8 peak bytes, 9 installations,
10 maximum primitive count, 11 maximum vertex count, 14 latest bytes.
Status0=OK,1=unknown opcode,2=unsupported model,3=pool pointer,4=allocation,
7=diagnostically disabled. Actual stats addresses depend on mod allocation order.

C11 warnings-as-errors tests pass, including model/index/opcode/pointer guards,
both frame buffers and restoration. All 4194304 aligned DMA tags are checked
with and without allocation. CPU aperture access uses physical address bits,
preventing unrelated high physical regions from aliasing the lower24 bits.
Regeneration adds one trusted entry hook; all41 generated C files match the
prior baseline after removing seven declared hook calls. No generated edits.

Native and Beetle SDK table hashes agree with the disc. Its quantization bounds
put the proposed2900 threshold plus a512-axis model and rounding allowance below
24192, safely below32767. The unchanged GTE flags continue rejecting invalid
projection. During6227 live ship passes status remained0 and the peak buffer
use was22724/196608 bytes. Exactly the two expected words differ from Beetle.
The captured visible state was attract/credits, so this is not a claim that a
user race or all cameras/tracks have passed. User visual confirmation remains
pending; the packaged release is unchanged. Proof index: ship-flash-manifest.json.

## Paused-race diagnosis and second candidate

The user-provided paused repro has stable camera and ship bytes. Ships0 and3
are straight ahead at eye depths22115 and18367, inside metric2900, but rejected
by section deltas-52 and-40. The original admitted window is-39..39. Native and
Beetle bytes agree at80021E68:2C42004F. The optional second candidate changes
that compare to2C02004F (SLTIU v0,zero,79); the surrounding branch/delay slot,
distance bound, hidden/damage and mode handling are preserved. Three-word
validation is atomic and diagnostic disable restores all three originals.
All65536 section inputs and restoration/opcode/model/pool tests pass.

The single-byte equivalent was applied to the running first candidate
for testing and restored after the user reported the result remained insufficient. The paused game reuses its frozen image; no ship-render call occurred
during the paused A/B probe. Later ring entries came from another caller and
are not evidence of newly visible opponents. User could not perceive a clear
improvement and still sees ships appear well after the track. This is NOT fixed.
Several opponents in the original repro lie beyond metric2900, at eye depths
27474..52752. Increasing the section window cannot address these.

Far-render requirements: calculate current ship rotation from angles70/72/74,
not cached model nodes (distant nodes are stale); preserve GT3/GT4 Gouraud colors
and texture fields (ship renderer11DE8 differs from scenery10000); preserve its
AVSZ depth bias-120; merge added ships with the existing sorted far-road/scenery
packets before insertion into the saturated far OT bucket. Appending a separate
ship list after scenery would not preserve cross-object depth order. Expansion
must retain bounded allocation and original near drawing. No beyond2900 live
range is authorized by current projection proof.

Transform evidence: Ghidra1E4FC rotation formulas match all five currently updated
model nodes in a stable paused read; ten distant nodes have stale position and
rotation. Native/Beetle quarter-wave tables match. Raw evidence is ignored under
analysis/verification/ship-transform-paused.json. This is a transform check, not
yet far rendering or GPU packet parity. Full pipeline analysis remains in progress.

A standalone ship-packet encoder draft (src/ship_packet.h) now preserves all eight
polygon classes, including GT3/GT4 colors, UV, CLUT and TPAGE. Synthetic format,
length, transparency and explicit GT4 field tests pass. It is NOT linked or enabled:
original-game packet comparison and combined far-scene depth ordering remain
unverified. The independent Beetle server has no gpu_ring_stats/frame_dump command;
it exposes RAM and call traces, so packet evidence needs an original DMA-list
capture through that harness before activation. No guessed far range was installed.

## Far-ship candidate built (next verification stage)

The candidate now adds far ships through the same collection and depth sort as
road and scenery. All slots are52bytes and total combined DMA allocation is
1671168bytes within the proven2MiB aperture. Near meshes retain the original
renderer below metric2900, with section-only culling removed. Far meshes use
current positions/angles, the verified transform hierarchy, int64 projection,
160..64000 camera depth and bounded GPU coordinates. Ship AVSZ bias-120 remains.
Normal-race hidden/damage rules remain; ghost and split-screen far coverage is
explicitly excluded pending its own validation. Original exhaust is unchanged.

Original Beetle buffers matched112 encoded polygons (82FT3,30FT4) across semantic
color/UV/CLUT/TPAGE and supplied screen-coordinate fields; unused GPU padding is
masked. All1302 records in the15 loaded models wereFT3/FT4. This proves encoding,
not new projection or ambient-light selection: those values are supplied to this
comparison. Tests additionally cover joint far-road/ship ordering, capacity
rollback, invalid indices, disabled behavior and near/far exclusion. Existing
road/scenery/billboard tests pass after changing their allocation stride to52.

RelWithDebInfo executable:build-win64/Wipeout2K97XL-ship-far-test.exe, running on
OpenBIOS TCP4370, PID12348. Canonical capture confirms title/intro. User race
appearance/occlusion is pending. Telemetry at9F000000 words22/23 exposes far-ship
packet count/status. This is a candidate, not a packaged or visually certified fix.
