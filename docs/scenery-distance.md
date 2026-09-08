# Experimental scenery distance extension

**Status: mesh extension significantly improved by user feedback; version-3 billboard candidate running for first-hill assessment.**
The 2026-09-08 resumed build uses version 3 with OpenBIOS LLE and the local
opt-in enabled. The public catalog remains default off; no release promotion.

The user reports that road distance is better but scenery/buildings/signs still
appear late on Talon's Reach. This extends the authorized draw-distance
experiment; it is not a claim that all pop-in is fixed.

## Recovered behavior

Ghidra 0x80046594 builds a section-dependent list with a 128-entry temporary
array. 0x80020128 applies sphere/frustum and far-table bounds before calling
0x80010000. The latter rejects projected primitive vertices using
`(FLAG >> 12) & 0x466`, including transformed IR3 overflow. The independent
Beetle GTE source (`Lm_B_PTZ`) sets that flag beyond signed 16-bit depth.
A live native trace records 45 IR3-rejected vertices from object renderer
callers 0x8001004C/0x80011E38 in one bounded sample.

A validated Talon's Reach inventory contains 436 models: 499 type-6 triangles,
4701 type-8 quads, 22 type-2 triangles, 538 type-4 quads, 18 flat quads,
18 Gouraud quads and 32 type-11 billboards. The ordinary scene renderer emits
flat-textured packets for source types 6/8. A separate track-index-4 inventory
contains 340 models. All vertex bytes and primitive types/flags/indices match
between independent native and Beetle captures of that track. This is geometry
agreement, not rendered-frame parity. Source colors/UVs can animate and were
excluded from that static comparison.

The initial sequential RAM capture can cross a scene transition. The inventory
tool rejects invalid pointers, unknown types and changing geometry ownership;
it compares two bounded RAM sweeps. Transform caches and animated record fields
are not claimed to be frame-atomic.

## Version 2 implementation

The default-off Extended Draw Distance experiment now adds ordinary scenery
meshes as well as road faces. An activation-only entry hook at 0x80010000 records
which models the original pass submitted this frame. Additional faces qualify
when the original model was omitted, or a participating vertex exceeded the
original input/transformed range. Eligible original geometry is not duplicated.

Static scenery must use the verified hierarchy: model -> identity camera-offset
node -> root camera. Rotation and translation composition follow 0x8001E8A8,
including separate fixed-point rounding for each parent level. Unknown hierarchy,
primitive records, indices and capacity violations discard the added scenery pass
and report a diagnostic error; original game rendering continues.

Original record types 1..8 are decoded into F3/FT3/F4/FT4/G3/G4 packets with
original colors, UVs, CLUT, TPAGE, semitransparency and conditional NCLIP policy.
Version 3 also extends anchored billboard types 10/11 using the game’s
texture descriptors and quarter-wave trigonometry table. Missing frame roll
metadata leaves billboards in the original renderer and is counted explicitly. Projected vertices must have depth 160..64000 and fit signed GPU
screen coordinates. Full polygon clipping across those limits is still open.

Road and scenery packets share sorting before insertion in the existing OT.
Scenery preserves AVSZ3/4's ZSF scale; far buckets clamp below the verified
background bucket. Two 491520-byte buffers use 983040 bytes of the opt-in DMA
aperture. The game heap and original ordering-table allocation are unchanged.

The road eligibility test also now checks transformed coordinates. Previously,
inputs could fit signed 16 bits while camera rotation made depth exceed 32767;
the original renderer rejected those faces, but the first prototype omitted
them. A directed rotated-vertex regression covers this gap.

## Verification and remaining limits

Synthetic Clang tests pass: source packet layouts, mixed road/scenery ordering,
no duplication of an eligible submitted model, addition of an omitted model,
invalid-record rollback, coordinate and capacity boundaries, and a full mixed
12288-packet buffer. Comparing 41 generated C files against regeneration without
mod hooks finds exactly five callback calls and no other differences. Generated
code is never hand-edited.

The earlier candidate run produced black captures before any extension pass, while
the user reported the main menu. Its root cause was not established. On the
2026-09-08 resume, both baseline and a fresh candidate boot produce valid captures;
the candidate then renders Talon's Reach in the attract/credits sequence. This
clears the immediate observation blocker without claiming a capture-tool fix.

A bounded Talon's Reach sample advances 916 rendering passes with road and scenery
status zero in all completed samples, up to 469 added scenery polygons, and six
coherent packet buffers passing layout, coordinate, capacity and link checks.
The renderer visits all 436 models and counts 32 unextended billboard records.
A fresh comparison of all 41 generated C files finds exactly five callback lines
and no other differences from the no-entry-hook baseline. Clang synthetic tests
and the Windows RelWithDebInfo build pass.

The candidate is left running for the user's first-track test. Physical race
feedback, stationary scenery-only A/B, seams, overpasses, occlusion and full
billboard coverage remain promotion gates. No updated release package was made.

## Diagnostics version 3

Read 128 bytes at 0x9F000000 after checking magic 0x444C5857 and version 3.
The original first sixteen words remain available, but packet count/capacity now
cover the combined pass. Word 16 is scenery enable (0/1), 17 last added scenery
packets, 18 billboards skipped for missing roll, 19 scenery status, 20 model count,
and 21 newly added billboard packets. Scenery
status 0 is success; 1 seen-model capacity; 2 camera hierarchy; 3 model/table;
4 transform range; 5 primitive record; 6 index; 7 packet capacity; 8 depth scale.
Word 8 still toggles the whole extension for diagnostics. A scenery-only A/B
changes word 16 so the user's accepted road extension remains active.

All RAM, model inventories, traces, generated code and captures remain ignored
local evidence; only original tooling and evidence summaries may be shared.

## Active candidate and recovery

CMake selects `src/track_distance_experimental.c` only. Its six configured hooks
include 0x80010000 and 0x80020128; do not link it together with `src/track_distance_mod.c`, since
both register the same plugin. The latter preserves the accepted road-only source.
Earlier restoration hashes in the manifest are historical recovery evidence,
not a description of the current candidate build.

The running candidate’s staged catalog describes the mesh extension;
the source catalog now includes billboards and will stage on the next build.
The local activation identifier is unchanged. Do not restart during user testing
just to refresh the label.


## First-hill pipe evidence and version 3

User feedback: the mesh extension is significantly improved, but 2D pipes pop
in straight ahead after the small first hill; some 3D pipe pop-in also remains.
The user paused immediately before that point. Saved local RAM and a visible
paused capture expose four type-11 sprites at depth 43272..44722. They are
outside the original transformed signed-depth range and were deliberately
unhandled by version 2. Version 3 emits their anchored FT4 quads and retains
original texture, color, near-depth and roll behavior. An offline replay of
that actual scene produces four packets within x=257..282, y=172..196.

Ghidra confirms the billboard decoder, trigonometric lookup and caller roll;
2048 bytes of the trigonometry table match independent live Beetle. Raw game
bytes are read at runtime, never embedded in mod source. Synthetic tests pass
for anchoring, quadrant signs, texture layout and rejection bounds alongside
existing mesh/full-capacity tests. Six generated callbacks are the only changes
across 41 C files compared with the no-hook baseline. RelWithDebInfo builds.

Live Talon's Reach sampling advances 880 passes with zero road/scenery errors,
no missing roll, an added billboard and six coherent buffers passing validation.
A subsequent capture visibly shows racing with the candidate active. The user
is testing the first-hill difference; the 3D pipe cause is not yet established.
No further 3D-range or clipping changes were made and no release was packaged.

Intermittent composed captures remain under diagnosis: after racing, composed
frames were black while the user reported the menu and canonical VRAM capture
showed visible menu pixels. Composed capture later worked again during racing.
This is not evidence of a game freeze or a completed capture-tool repair.


User verification: the first-hill 2D pipes now appear without pop-in. The user
cannot readily perceive other remaining pop-ins. This does not establish full
all-track coverage. Internal resolution is now 4x at the user’s request; live
GL wide-surface capture verifies 1704x960. See resolution-manifest.json.
