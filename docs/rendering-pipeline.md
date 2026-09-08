# Wipeout XL SCUS-94351 rendering pipeline â€” working analysis

**Coverage: partial static analysis, not an exhaustive or gameplay-verified pipeline.** The user requested the entire 3D pipeline for future work. This document preserves what is established and makes the completion requirements explicit. The allocator correction now permits native post-intro 3D captures. Controllable gameplay and matched-state full-frame comparisons remain pending.

## Input and evidence

Boot executable: SCUS_943.51; PS-X EXE entry 0x80082D70, payload loaded at 0x80010000. Payload SHA256: `92fecf36065150f7bfd274222f4c4bc101b1a4606e882edbe1f2d06713507aff`.

Ghidra MCP queried 881 recognized functions without query errors; 29 contain COP2/GTE commands or register transfers. `tools/render_inventory.py` reproduces the inventory against the local Ghidra program. Disassembly and decompilation remain local under ignored `analysis/rendering/`; the tracked [manifest](verification/rendering-manifest.json) contains only addresses, operation counts and call relationships.

Function boundaries are Ghidra's current discoveries, not a proof that every executable byte is covered. Static call edges omit unresolved indirect targets. Runtime overlays require separate image identity, relocation and reachability evidence. Inventoried functions must not automatically become verified recompilation seeds.

## Initial pipeline map

| Stage | Located addresses / facts | Evidence status |
|---|---|---|
| Display and projection setup | 0x8005789C sets 320Ã—240 rectangles at VRAM Y=0 and Y=256, screen center (160,120), projection distance 160 | Static call/constants; runtime gameplay confirmation pending |
| Scene/track selection | 0x8001F4FC and 0x8001FD10 call 0x80013C78, 0x800140AC, 0x80014AD4 | Static call edges and visibility-list accesses; exact near/far categories pending |
| Transform preparation | 0x8001E9F0 calls 0x8001E8A8, then 0x80080968 and 0x800808D8 | Matrix computation and GTE upload chain located; coordinate convention pending |
| Object primitive assembly | 0x80010000, 0x80011DE8, 0x80012ED4 | RTPS/NCLIP and depth-operation families; object/material dispatch details pending |
| Track primitive assembly | 0x80013C78, 0x800140AC, 0x80014AD4 | Projection, packet writes and texture lookup located; exact clipping/LOD policy pending |
| Additional projection/depth helpers | 0x80015CB8, 0x80015D1C, 0x800160CC, 0x80016110 | GTE operations identified; all caller roles pending |
| Ordering tables / packet buffers | 0x80094ADC, 0x80094B04, 0x80094C6C referenced by 0x80013C78 | Double-buffer selection and packet indexing located; full bucket/link policy pending |
| Submission, synchronization, presentation | Calls from display setup plus GPU/DMA paths | Full call chain, register transactions and per-frame proof not yet mapped |

These are descriptive working labels, not recovered original function names.

## Geometry and track records found

In 0x80013C78, the object referenced by 0x80094A10 supplies vertex, face and section bases at offsets +0x0C, +0x10 and +0x14. Address arithmetic uses a 0x10-byte vertex stride and 0x14-byte face stride. The calling section traversal uses a 0x9C-byte section stride. Section offsets +0x8E and +0x90 supply a first-face index and count. Signedness, complete field layouts, ownership and file formats still require verification.

The function allocates/indexes 0x28-byte packet records from a buffer selected using the frame-buffer index. It projects one vertex with RTPS and three with RTPT, reads projected coordinates and checks GTE flags. It also performs signed range checks before loading coordinate sums into the GTE. These checks must be mapped branch by branch before labeling them as clipping or overflow handling.

Texture lookup uses face metadata and a 0x1F8-byte record stride, with an alternate region at +0xFC. Texture format, page/CLUT ownership, animation and the full meaning of flags remain unverified. Do not infer a final material schema from those strides alone.

## GTE support routines

| Address | Observed operation family |
|---|---|
| 0x8007F570 | GTE control-register initialization |
| 0x8007F9E8, 0x8007FB00 | GTE data-register transfers; higher-level mathematical role unverified |
| 0x8007FE30 | Three MVMVA operations, matrix composition candidate |
| 0x800808D8, 0x80080908, 0x80080938 | Five control-register transfers each |
| 0x80080968, 0x80080C30 | Three control-register transfers each |
| 0x80080CE4 | Two control-register transfers; screen-offset setter used by setup |
| 0x80080CFC | Projection-distance setter used by setup |
| 0x80080E40, 0x80081330 | RTPS |
| 0x80080E6C | RTPT |
| 0x80080EC0 | MVMVA |
| 0x800811C4 | NCLIP |
| 0x800811F8 | AVSZ3 |
| 0x80081228 | AVSZ4 |
| 0x8008125C | OP |

0x80010000 contains 16 NCLIP sites and eight each of AVSZ3/AVSZ4; 0x80011DE8 contains eight NCLIP sites and four each of AVSZ3/AVSZ4. Counts describe static instructions, not per-frame work or polygon counts. Register selection, fixed-point scales, saturation, winding and depth bias must be recovered from each use site.

## Required coverage before claiming completeness

1. Enumerate executable disc modules and every runtime overlay; record source identity, load/relocation maps, function reachability and indirect dispatch targets. Include menus, attract mode, every track, ship, weapon, effect and replay path.
2. Recover scene, camera, ship and track transforms end to end, including coordinate spaces, fixed-point formats, matrices, projection parameters and camera changes.
3. Decode asset records, material/texture/CLUT placement, uploads and lifetime. Establish how visibility lists, section traversal and distance select geometry detail.
4. Map every primitive variant: flat/Gouraud, textured/untextured, triangle/quad, sprites/lines, transparency, fog/color processing, culling, clipping, subdivision, screen bounds and GTE overflow handling.
5. Reconstruct ordering-table size, bucket calculation, insertion order, packet links, buffer ownership and GPU DMA submission. Separate world geometry, HUD, backgrounds, menus and FMV display paths.
6. Capture a complete frame on independent Beetle and native at matched game state: transforms, GTE inputs/outputs, packet memory, OT traversal, GP0/GP1 stream, VRAM and displayed pixels. Check multiple cameras, tracks and effects rather than one quiet frame.
7. Derive enhancement integration points only after those proofs: 16:9 projection versus culling/LOD/HUD behavior, and CRT processing after faithful presentation. A stretched image is not verified widescreen.

## Runtime overlay evidence

The intro loads code beyond the original executable. A local snapshot at 0x80118000 covers 0x22000 bytes and is imported as `native-overlay-80118000.bin` in Ghidra. It includes the intro cleanup call to BIOS free, but it has not been established as a complete executable overlay or fully analyzed for rendering. Snapshot CRCs can change because the range includes writable state. Preserve code/data distinctions and compare against disc source before assigning module identity.

No game-derived source listings, binary data, textures, or captures are included in this tracked document.

## Additional verified static details (allocator-diagnosis continuation)

Raw instruction decoding confirms the control-register indices, avoiding Ghidra's encoded register-number presentation:

| Function | Registers | Role |
|---|---|---|
| 0x800808D8 | COP2 control 0..4 | Rotation matrix |
| 0x80080908 | COP2 control 8..12 | Light matrix |
| 0x80080938 | COP2 control 16..20 | Color matrix |
| 0x80080968 | COP2 control 5..7 | Translation, sourced from matrix record +0x14/+0x18/+0x1C |
| 0x80080C30 | COP2 control 13..15 | Background color, input channels shifted left four |
| 0x80080CE4 | COP2 control 24..25 | OFX/OFY, inputs shifted left sixteen |
| 0x80080CFC | COP2 control 26 | Projection distance H |

The 0x80013C78 track path projects vertex 3 separately, then vertices 0..2 together. Its packet coordinate write order is vertex 1 at +0x08, vertex 0 at +0x10, vertex 2 at +0x18, vertex 3 at +0x20. It takes RGB from face-record +0x10; UV pairs occupy +0x0C, +0x14, +0x1C and +0x24. Two additional 16-bit texture descriptor fields go to +0x0E and +0x16. This layout is consistent with a flat textured quad, but the command-byte/length initializer has not yet been recovered, so final primitive classification remains pending.

Depth comes from COP2 data register 19 (SZ3), read at 0x80013FA4. The OT byte offset is `SZ3 & 0xFFFC`. The path prepends the packet by copying the previous 24-bit bucket link into its tag, preserving the tag's high byte, then storing the packet's low 24 address bits in the bucket. Therefore this path uses the last projected vertex depth, not an AVSZ result. Other located helpers at 0x80015CB8 and 0x800160CC use AVSZ4; they must not be collapsed into one universal sorting formula.

The texture descriptor alternative is selected by bit 2 of face-record byte +0x0F; the descriptor index is byte +0x0E, and the alternate lies +0xFC within a 0x1F8-byte record. These offsets refine the earlier working description. Semantic texture-page/CLUT naming still requires the initializer and upload chain.

This is static evidence only. Runtime OT allocation bounds, packet initialization, all primitive variants and matched native/Beetle frame captures remain required.


## Frame submission and GPU driver (2026-09-07)

Ghidra resolves the live DrawOTag caller to **0x800684C4**. It waits through
DrawSync (0x8007BE24), performs conditional vertical synchronization through
0x800820D8, then flips 0x80094C6C with `(index + 1) & 1`. It installs the new
buffer's draw/display environments through 0x8007C3A4 / 0x8007C4FC, using the
pointer pairs at 0x800BC8E8 / 0x800BC8EC. Submission uses the opposite index:
main OT base from 0x80094ADC plus 0x7FFC; conditional secondary base from
0x80094B28 plus 0x18FC. These are terminal-word offsets for 8192 and 1600
four-byte entries respectively; allocation and every producer still need
verification. Return addresses 0x8006865C and 0x800686A0 appear in native
DrawOTag tracing. The secondary table's content must not yet be labeled HUD.

The pointer at 0x80091838 selects the GPU driver table at 0x800917F8:

| Table offset | Target | Verified operation |
|---|---|---|
| +0x04 / +0x08 | 0x8007DB64 / 0x8007DB88 | Queue wrapper / enqueue |
| +0x10 | 0x8007DA40 | GP1 write and software command shadow |
| +0x14 | 0x8007DA98 | Direct GP0 word submission |
| +0x18 | 0x8007DAE8 | Linked-list GPU DMA submission |
| +0x20 | 0x8007D4E4 | CPU-to-VRAM image upload |
| +0x24 | 0x8007DDF0 | Queue pump |
| +0x2C | 0x8007D1B0 | Reverse ordering-table clear via DMA6 |
| +0x3C | 0x8007E26C | Drawing synchronization |

Table word zero points to data at 0x80019C58, not a function. DrawOTag at
0x8007C334 enters the queue wrapper with the linked-list callback. The queue
uses 64 slots with a 0x4C-byte stride, optional packet copying and DMA2 busy
checks. PutDrawEnv constructs an environment packet with a terminal link and
queues its copy; PutDispEnv writes GP1 display start/ranges/mode directly.

The linked-list callback sets GP1 to 0x04000002, DMA2 MADR to the root, BCR to
zero and CHCR to 0x01000401. OTC initializes DMA6 MADR to `base + count*4 - 4`,
BCR to count and CHCR to 0x11000002. The image uploader emits GP0 0xA0000000,
coordinates and dimensions, writes a remainder directly, then sends 16-word
blocks through DMA2 CHCR 0x01000201. Texture ownership, CLUT relationships and
all upload callers remain to be recovered.

A bounded native capture contains **705 GPU commands at frame 28463**, with
packet source addresses and submission PCs. Another bounded DMA capture
confirms channel-2 starts at PC 0x8007DB28 and channel-6 starts at 0x8007D220;
the latter includes a 1600-word OTC at 0x800F3E8C. DMA ring `words` for a
linked list is not a measured polygon count. GPU capture payloads are limited
to 12 words per command and do not establish complete large image uploads.
RAM write tracing does not instrument MMIO; the dedicated `dma_trace_dump`
command provides the DMA evidence. The empty MMIO-targeted RAM trace is not
used as proof.

These captures are from asynchronous native states. They do not establish a
matched native/Beetle frame or resolve the earlier corrupt transition frame.
Full scene, primitive and overlay coverage remains open.

## Transform hierarchy and directional track lists

Instruction verification of 0x8001E8A8 resolves a missing argument in Ghidra's
decompilation: the recursive call receives the pointer loaded from node+0x44.
A root returns its local matrix at +0. A child returns its cached composition
at +0x20; dirty halfword +0x40 triggers recursive parent resolution. Translation
at +0x14/+0x18/+0x1C is multiplied by the parent's signed halfword 3x3 matrix,
with 32-bit products/sums and arithmetic shift 12, then parent translation is
added into +0x34/+0x38/+0x3C. Matrix composition calls 0x8007FE30 and clears the
dirty flag. 0x8001E9F0 uploads this resolved translation and rotation to GTE.
The node's association with each camera/scene owner still needs tracing.

0x8001F4FC chooses one of four list families through 0x8002005C. Each list is a
sequence of signed halfword pairs: section index, renderer parameter. Section
addresses use the verified 0x9C stride. For selector 1, renderer 0x80013C78 uses
pointer/count +0x2C/+0x64, 0x800140AC uses +0x28/+0x62, and 0x80014AD4 uses
+0x24/+0x60. Selectors 2, 4 and 8 use the corresponding triples at pointers
+0x30..+0x38, +0x48..+0x50 and +0x3C..+0x44. Counts count halfwords, so the
loop advances by two. The first renderer's parameter is capped at 100 in this
path. 0x8001FD10 instead traverses the triple +0x54/+0x78, +0x58/+0x7A,
+0x5C/+0x7C without that cap.

0x8002005C computes an angle from X/Z differences between linked records,
subtracts it from the caller's signed heading at +8, normalizes through
0x80025528, and selects quadrants using 0x200/0x400/0x600 boundaries. This
establishes direction-dependent precomputed section lists. It does not yet
establish their generation, visibility conservatism or suitability for 16:9.


## Object renderer atlas (static analysis; runtime parity pending)

The three object renderers at 0x80010000, 0x80011DE8 and 0x80012ED4 share
an instruction-verified object header: unsigned vertex count +0x10, vertex
pointer +0x14, unsigned primitive count +0x20, primitive pointer +0x24 and
transform-node pointer +0x30. Vertex records are eight bytes. Projected
records contain SXY at +0, SZ3 at +4 and `(FLAG >> 12) & 0x466` at +6.
Up to 128 vertices use scratchpad 0x1F800000; larger objects use 0x800C692C.
Each vertex executes RTPS. The shared packet cursor is 0x80094A48; the active
OT comes from 0x80094ADC indexed by 0x80094C6C.

The following table records the decompiled dispatch paths. Opcodes are base
values before optional semitransparency. It is a static atlas, not proof that
every variant has executed or matched Beetle.

| Record type | Record bytes | 0x80010000 | 0x80011DE8 | 0x80012ED4 |
|---|---:|---|---|---|
| 1 | 16 | 0x20 F3 | 0x20 F3 | 0x20 F3 |
| 2 | 28 | 0x24 FT3 | 0x24 FT3 | 0x24 FT3 |
| 3 | 16 | 0x28 F4 | 0x28 F4 | 0x28 F4 |
| 4 | 32 | 0x2C FT4 | 0x2C FT4 | 0x2C FT4 |
| 5 | 24 | 0x30 G3 | 0x30 G3 | 0x30 G3 |
| 6 | 36 | 0x24 FT3 | 0x34 GT3 | 0x24 FT3 |
| 7 | 28 | 0x38 G4 | 0x38 G4 | 0x38 G4 |
| 8 | 44 | 0x2C FT4 | 0x3C GT4 | 0x2C FT4 |
| 10, 11 | 16 | CPU-expanded textured quad | CPU-expanded textured quad | Not handled |

0x80010000 selects a faster dispatch loop when all projected flags are zero;
otherwise it checks the participating vertices. 0x80011DE8 checks each
primitive and retains Gouraud texturing for types 6 and 8. NCLIP is conditional
on record flags in these paths. 0x80012ED4 requires positive NCLIP and uses
the caller-supplied third argument as a fixed OT index, unlike the AVSZ3/4
sorting of ordinary triangles/quads in the other paths. Semitransparency
handling needs a branch-by-branch audit; it must not be assumed universal.

Types 10/11 expand an anchor into a rotated screen-space quad using CPU
arithmetic, sine/cosine and the constant 160 divided by anchor depth. They
reject flagged anchors and depths below 0x800. Record +4 selects the anchor;
+6/+8 give dimensions; +0x0A indexes the texture-pointer table at 0x800CE468;
+0x0C supplies color. Raw instructions at 0x80011CAC onward verify this
texture table, packet command 0x2C, nine payload words, descriptor +2 to
packet TPAGE, descriptor +4 to packet CLUT, and UV halfwords +0x16 through
+0x24 narrowed to bytes. Sorting uses anchor SZ3 masked with 0xFFFC. The
semantic owner of each billboard type is not yet established. These CPU
expansions are a separate widescreen concern from the GTE projection.

A bounded native TCP sample observes 60 calls to 0x80010000 and two each to
0x80011DE8 and 0x80012ED4. The early independent Beetle sample was invalid for renderer coverage: the
probe omitted 0x prefixes, so numeric targets were parsed as decimal. The
corrected, read-back-validated probe now observes all seven rendering entries
on each backend. This proves call coverage, not packet or pixel parity. Raw calls and
arguments remain in ignored `analysis/rendering/object-entry-traces.json`.
Tracing used each server's implemented arm-list/disarm command names and
restored the previous watch lists. No guest data or generated code changed.

Remaining verification: primitive packet contents at matching checkpoints,
all clipping and transparency branches, texture ownership/uploads, camera
owners, track subdivision, effects/HUD, dynamic overlays and matched frames.


## Track subdivision, allocation and texture layouts

Raw instructions and Ghidra decompilation resolve the three section renderers:

| Entry | Mesh per face | Sorting | Packet pointer pair | Capacity per buffer |
|---|---|---|---|---:|
| 0x80013C78 | Original quad | Last projected SZ3 & 0xFFFC | 0x80094B04 | 560 |
| 0x800140AC | 3x3 vertices, four quads | AVSZ4 per quad | 0x80094BE8 | 112 |
| 0x80014AD4 | 5x5 vertices, sixteen quads | AVSZ4 per quad | 0x80094AF8 | 448 |

0x80062484 allocates two arrays of each size (0x5780, 0x1180 and 0x4600 bytes)
through 0x8001D688. It initializes every 40-byte record using 0x8007B598,
which writes tag length 9 at +3 and GP0 opcode 0x2C at +7. This closes the
previous uncertainty over the original track path's primitive initializer.
Counters are unsigned halfwords at 0x800944E4/E6/E8. Ghidra misses some
GP-relative xrefs: the stores at 0x80062550/57C/5AC use GP displacements
0xD20/0xE04/0xD14 with GP 0x80093DE4. Instruction verification is necessary.

The 2x2 and 4x4 paths add camera-record +0x14/+0x18/+0x1C to 32-bit world
vertices and construct midpoint grids with arithmetic shifts, then narrow
coordinates to signed halfwords. Each grid point executes RTPS and stores
SXY, SZ3 and the reduced FLAG mask in a 16-byte scratchpad record. An all-bad
grid is rejected. An all-good grid takes an unrolled emission path. Mixed
grids check participating vertices individually. The 4x4 renderer additionally
splits a mixed subquad through 0x80015D1C: five new position/UV midpoints,
five RTPS operations, then up to four child quads with zero reduced flags.
It does not recursively invoke itself. This is subdivision/rejection, not
an analytically computed intersection with a clipping plane.

0x80015C64 fills ordinary subdivided packets. 0x80015CB8 performs AVSZ4,
links the packet to the selected OT bucket, and advances the dedicated cursor
and counter. The extra-split helpers 0x80016074/098/0CC instead initialize
0x2C/9, write geometry/UV/material/color, sort with AVSZ4 and advance a cursor
loaded from 0x80094A48. These helpers pass state in registers outside the
standard C argument set. In particular, 0x80014AD4 copies its section count
from a3, whereas 0x800140AC uses a2; inferred prototypes must not be used
as hook signatures without checking the actual instructions.

Texture builder 0x80062484 invokes 0x80061FF8, 0x80062134 and 0x800622DC
for each texture, in both orientations. Each 0x1F8-byte entry at 0x800C39EC
has two 0xFC-byte halves. Each half contains 21 twelve-byte descriptors:
one whole-face descriptor at +0, four subdivision descriptors at +0x0C,
and sixteen at +0x3C. The source mapping is a 0x2A-byte record with sixteen
indices at +0, four at +0x20 and one at +0x28, each resolving through the
texture-pointer table 0x800CE468. Descriptor +0 is TPAGE, +2 CLUT, and +4..+11
are UV bytes copied from source halfwords +0x16..+0x24. The alternate half
reverses each row's index order and swaps left/right UVs. Face byte +0x0F
bit 2 selects that half. Texture image allocation and upload ownership
remain separate unfinished analysis.

`tools/capture_render_calls.py` now validates armed addresses by reading them
back and uses explicit hexadecimal prefixes on both servers. All seven
main renderer entries have runtime call evidence. `tools/verify_track_pools.py`
uses a renderer call's frame plus the following frame-history snapshot,
checks the pool pointers, and verifies usage counters and the first packet
in both buffers. All three pool levels passed in both backends. These are
six packet-header checks per backend, not a full-pool or matched-frame proof.
Earlier asynchronous pool reads and snapshots gated only by counters are
invalid evidence: allocations can change and counters can persist outside
rendering. Those observations prompted the call-linked sampler; no guest
code was changed to accommodate them.

## Particle lines and display configurations

0x80016110 visits 127 records of stride 0x44. Active records hold signed
halfword velocity at +2/+4/+6, 32-bit position at +8/+0x0C/+0x10, prior
projected XY at +0x14 and lifetime at +0x18. When its third argument is below
one, it integrates velocity and decrements lifetime. Five type-dependent
color paths feed a two-color line packet: GP0 0x52 and four payload words,
using current and previous projected endpoints. Each record carries two
20-byte packet regions selected by the frame buffer index. The initial
projection primes the previous point before a line is linked. It rejects
out-of-halfword-range coordinates and FLAG & 0x80460000; its OT depth comes
from IR0 shifted right one, not the track/object SZ3 or AVSZ formulas. The
semantic owners of all five particle types still need recovery.

Four Ghidra-located callers set projection center/distance:

| Setup entry | Display dimensions | OFX, OFY | H |
|---|---|---|---:|
| 0x8005789C | 320x240 | 160, 120 | 160 |
| 0x80068214 | 320x180 | 160, caller-supplied Y | 160 |
| 0x80068318 | 320x240 | 160, 120 | 160 |
| 0x8006842C | 640x256 | 320, 128 | 320 |

The latter three call the display environment builder at 0x8007A828 and
PutDispEnv at 0x8007C4FC. Their scene ownership and transitions must be
verified before selectively widening them. Projection, CPU-expanded
billboards, precomputed visibility lists, subdivision/rejection and the HUD
cannot yet be treated as one proven widescreen mechanism.

## Scene-object visibility and widescreen gate

Ghidra decompilation and all 193 instructions of 0x80020128 establish a
CPU visibility test before the object renderer. The section object list
comes from the second argument's +4 pointer: signed count at +0x20,
indices at +0x1C. Each index selects a pointer through 0x80094A80. The
object transform at +0x30 is marked dirty and uploaded through 0x8001E9F0;
0x80080EC0 transforms the zero vector to obtain its camera-space center.
The extent is the object's word at +0x34.

Depth acceptance requires Z + extent > the near threshold and
Z - extent < the far threshold. Threshold pairs come from 0x8008E490 /
0x8008E4B0, or 0x8008E4D0 / 0x8008E4F0 when 0x80094C44 is two, indexed
by the halfword at 0x80094B8C. The projected X, Y and extent each use
signed integer multiplication by 160 and division by Z + extent.
Instructions 0x80020388..0x800203A4 accept X inclusively within
plus/minus (projected extent + 160). Instructions 0x800203A8..0x800203C4
apply the corresponding Y bound with 120. Accepted objects call
0x80010000 at 0x800203D0. This is independent of the GTE projection
registers and of the host presentation rectangle.

Consequently, a wider host surface alone does not widen this CPU test.
The amount of missing geometry in a 16:9 race remains to be measured;
the precomputed track lists are a separate visibility gate. No guest
instruction or generated code has been changed. The local 16:9 config
probe remained mode 0 / extra width 0: main.cpp intentionally clamps
legacy aspect settings to 4:3 until trusted widescreen mod activation.
That probe is not evidence of working widescreen.

Main routine 0x8001A504 calls the 640x256 setup before the SCE logo and
returns to 320x240 before the copyright presentation. It loads
ntscanim.exe and later XTRO overlays through 0x8006726C. The title loop
clears both ordering tables, submits background/text and calls the shared
frame submitter. These static call relationships identify additional
coverage required: overlay rendering, scene transitions and the secondary
ordering table's producers. The secondary table has menu, race and other
callers and must not be labeled exclusively as a HUD table.

The local object-cull baseline proof compares all 0x304 bytes of this
routine against the extracted disc executable in both running backends.
Both match. Bounded, address-readback-verified function watches observed
16 native and 14 Beetle calls, then restored the previous watch lists.
This verifies code identity and execution, not individual branch outcomes
or equivalent visibility at a matched camera position.

## Optional 16:9 implementation

The default-disabled `wxl.presentation.widescreen` mod selects the existing
OpenGL native-wide presentation and GTE-activity scene detector. It keeps
GTE projection geometry unchanged and expands the destination surface to
426 pixels for a 320-pixel game display. The verified ADDIU at 0x80020388
uses the existing bias-site transform, adding the runtime per-side margin
(53 pixels in this mode) to the horizontal extent. The vertical extent,
projection scale, near/far thresholds and precomputed track lists are unchanged.
This is an optional enhancement, not a correction to original PS1 behavior.

The emitter's bias-site early return previously omitted normal PGXP ALU
transport. It now uses the shared hook emitter, matching the existing
dynamic instruction path; a regression check covers its presence. Baseline
regeneration confirms one expression differs in one shard. Setting its
margin to zero restores the baseline source calculation and metadata hook,
apart from indentation. No generated source was edited by hand.

A final-build completed presentation capture shows a snowy demo race across
the wider surface with centered text. Track-pool samples pass all three
capacities and both buffers' first packet headers. The earlier credits
capture also shows geometry across the expanded surface. These observations
do not prove every track or racing HUD state: physical race verification
and broader camera/overlay coverage remain open. See
`docs/verification/widescreen-manifest.json` for source and proof hashes.


### Talon’s Reach pipe follow-up (2026-09-08)

The first-track inventory contains 62 `keypipe` models, each four type-8
textured quads (248 pipe mesh primitives total), plus 32 type-11 billboard
primitives. These categories must not be conflated when diagnosing pop-in.
The user's paused downhill view exposes a four-billboard group at depth
43272..44722, beyond the original transformed signed-depth limit. The earlier
mesh-only extension omitted this group entirely.

Ghidra 0x80010000's type-10/11 paths use one projected anchor vertex, source
signed halfword width/height (+6/+8), texture index (+10), and RGB (+12).
Projected width is halved after integer division; height is not halved.
0x80020128 supplies roll 0 unless its state flags at +12 contain bit 4,
then uses the signed halfword at +0x74. Its JAL delay slot sets a2, so a
Beetle call-site trace's pre-delay a2 is not the callee's roll argument.
The candidate captures the caller's state at entry to avoid that ambiguity.

0x8007EF1C/0x8007EF6C (sine) and 0x8007F024 (cosine) index the game's
quarter-wave table at 0x80091880. All 2048 table bytes agree between the
paused native inventory and independent live Beetle. Type 10 anchors the
upper edge; type 11 anchors the lower edge. Both emit opaque FT4 regardless
of source flags, read texture descriptors through 0x800CE468, and use
anchor depth >> 2 for OT indexing. Descriptor +2 is TPAGE, +4 CLUT, and
+22..+36 hold eight halfword UV components whose low bytes enter the packet.
The original minimum billboard depth is 2048. The extension retains these
rules while allowing the established extended depth range.

The captured hill scene yields four bounded packets at x=257..282,
y=172..196 (original 320x240 coordinate space), with original texture/color
fields. This is offline packet evidence; actual hill-view gameplay and
3D pipe rejection transitions still require separate observation.


## Weapon-hit display shake (2026-09-08)

The user paused immediately after hitting mines. TCP reports a 320x238 scanout,
GP1 vertical range [18,256], drawing area [0,0,319,239], and unchanged projection
H=160, OFX=160, OFY=120. Both DISPENV records (800D0620 and 800D0634) retain
320x240 nominal dimensions; their screen origins are (-2,2). The distance
plugin reports rejection status 3 and zero new packets. Its scenery counters
are stale on this early return and must not be treated as current geometry.

Ghidra function 8004B53C randomizes the two DISPENV screen origins and restores
them as its shake counter expires. PutDispEnv at 8007C4FC converts screen.y=2
to start=18 and end=258, then clamps end to 256. This explains the measured
238 lines without any change to scene projection or geometry. Setup 80068318
establishes the nominal 320x240 rectangles. A live Beetle read confirms those
nominal rectangles too; a matching Beetle damage event remains unverified.

The candidate validates both nominal scene sizes, retaining the existing GTE
projection checks. Synthetic tests cover the captured shake and reject a
180-line scene in either buffer. RelWithDebInfo candidate builds successfully;
it has not replaced the user's paused executable. See damage-display-manifest.json.

The OpenGL full-width flat-overlay path also queued triangles while mirror
suppression was active but released suppression before flushing them. The
candidate explicitly flushes at both ownership boundaries. Visual verification
of victory-screen borders remains pending, as does loading-screen blue fill.


## Ship submission investigation (2026-09-08)

Ghidra 80021B90 iterates 15 ship records, stride 0xF0. The mesh branch requires
its camera-relative distance metric below 1200 and positive projected depth,
unless the ship is the selected local ship. Coordinates are shifted right by
three before 80025484 sums their squares and calls SDK approximate square root
8007F9E8. A separate 2900 threshold controls a broader visibility bit. These
are distinct from the scenery draw-distance extension.

The mesh branch additionally checks a cyclic track-section delta in [-39,39],
hidden/destroyed flags, and race modes. It updates model transforms and invokes
80011DE8. That renderer preserves Gouraud-textured triangles/quads, including
52-byte GT4 packets, and subtracts 120 from AVSZ depth buckets. The current
scenery extension's 40-byte packets cannot represent all these ship primitives.
Far model transforms must not simply reuse nodes updated only by the near branch.

The 0x688 dispatcher bytes match in native and Beetle RAM (manifest:
verification/ship-flash-manifest.json). Native call coverage is observed; the
initial Beetle sampling was on credits and did not observe this race dispatcher.
No ship cutoff has been changed: packet capacity, far depth and visual behavior
remain verification gates. User has confirmed the earlier damage and victory
fixes; the newly reported thunder-bomb side flash awaits its own command capture.

## Far moving ships: updated evidence

See enemy-ship-distance-proposal.md and verification/ship-flash-manifest.json for
current candidate status. All1302 records in the15 loaded models are FT3/FT4;
112 original Beetle-buffer polygons match the encoder's meaningful fields.
Distant model nodes are stale; current rotation is calculated from angles70/72/74
with the original trigonometry table. The hierarchy composes camera offset and
ship translation in separate fixed-point steps. Far ships join road/scenery
before depth sorting and preserve the ship AVSZ bias. Slot capacity is52bytes.
Near meshes retain original rendering. User far-visibility and occlusion testing
is pending. The full pipeline audit remains incomplete, including ghost,
split-screen and exhaustive effect coverage.

## Exhaust and shadow paths (verified September8)

User confirms extended ship meshes now work well. The remaining exhaust cutoff
is separate:726AC tests ship.flagsC bit200 and clears the11-entry trail ring when
it is absent. Each ship uses900hex bytes at the pointer in80094D34; records start
at10hex withD0hex stride.70EB0 decrements RGB by27 (clamped0), moves the head
backwards modulo11, stores two nozzle-edge world positions and selects one of
two RGB palettes scaled by min(ship94/2,255). The anchors are ship position minus
forward-vector*420/4096, y minus15, plus/minus side-vector*40/4096.71108 is the
local first-person variant and remains unchanged.

720E0 gates trail drawing with bit200 and hidden state.71970 chooses the close
ribbon or71418 beyond metric900. The distant path joins every second ring point,
uses a two-screen-pixel G4 strip, duplicates endpoint colors across its width,
and applies additive state E1000620. Its depth comes from the last RTPT SZ/4,
minus160, then divided by2 for the OT slot.80E6C confirms this return convention.
The depth-cue threshold4000 reduces distant segments to the head strip. Raw
native/Beetle buffers contain13 matching strip records,11 in Beetle. The detailed
head flare helpers6F8AC/6FFAC are distinct and remain original.

The enhancement captures726AC entry and keeps a presentation-only shadow ring
for far ships. Near data is copied from the original history; far anchors and
fade follow the verified mechanism. A separate periodic1/8 accent selects the
same palettes without consuming game RNG or changing simulation visibility bits.
Far strips enter the shared geometry sort, with bounds and state checks. Tests
assert zero guest RAM changes from capture, fade/anchor arithmetic, packet shape,
state and capacity rollback. User visual verification of this candidate is pending.

4A2A0 is the projected ship-shadow grid, not the exhaust: it projects four ship
footprint corners onto the track face plane and subdivides a4x4 screen-space grid.
It has its own visibility gates and175/195/100 depth offsets.16110 is a127-record
particle-line path (44hex stride), with52hex Gouraud-line commands, lifetime
updates and signed16 projection checks. Neither was changed for exhaust extension.


### Distant exhaust subpixel presentation (2026-09-08)

User reports the extended ribbon becomes an oversized blue rectangle. The
original 80071418 disassembly and 13 captured native/Beetle packets prove a
fixed two-native-pixel width, copied by the first extension. This is retail
effect design, not a resolution-dependent fault in the faithful renderer.

The optional extension now supplies explicit 16.16 endpoint positions and
width `2 * 23200 / max(z,23200)` native pixels. This enhancement policy
matches the old width at the forward metric-2900 handoff (world scale eight),
then shrinks to 0.725 pixels at z=64000. It is not claimed as retail behavior.
The same colors, ring cadence, additive blending and OT convention remain.

A generic runtime API attaches value-checked precise positions only to the
allocated enhancement DMA aperture. Guest aperture writes invalidate shadows,
including byte/halfword writes, and boot clears them. Ordinary RAM cannot
acquire these overrides. GPU triangle preparation consumes them independently
of PGXP configuration; OpenGL already preserves 16.16 fractions through its
float rasterizer. The integer packet envelope remains two pixels. Host packet
sorting carries precision alongside its packet; no generated code changes.
Stats word26 at 0x9f000068 counts successful runtime precision lookups;
trail status3 means precision attachment failed.

Tests cover monotonic perspective width, fractional negative projection,
aperture aliases/allocation/value checks and invalidation, plus existing
packet, guest-RAM-identity, bounds and rollback checks. Live race verification reports 18,888 precision lookups, ten far trail
polygons and status0. User confirms the revised width looks proper.


## Loading and front-end presentation (2026-09-08)

Loading frame24158 fills both320x240 buffers with GP0 color022E0000,
then updates a320x100 band at y80/336. Without sustained GTE work, the
45-frame detector expires and GL presents centered4:3 with black panels.
An opt-in uniform-clear background tracker now colors those panels from
the native clear. It tracks buffer rows independently; unknown/mixed rows
and FMV do not supply a color. The canonical artwork and VRAM are unchanged.
Twelve composed captures succeeded and the user confirmed loading looks good.

The rotating ship preview in team selection independently keeps the GTE
detector alive, revealing repeated menu panel art outside4:3. GPU state on
that screen has solid_background0, so it is not caused by the new fill rule.
Ghidra main8001A504 calls menu8004DA10 and enters racing/attract through
8003EA68. The former runs the option loop, including preview rendering; the
latter allocates the race structures. Race-type global80094C44 is already0
in team selection, so it cannot distinguish UI from gameplay.

Two trusted widescreen-only entry callbacks now select centered UI mode on
menu entry and release it on race entry. No guest writes. First256 bytes at
both functions match the disc executable independently in native and Beetle.
Regeneration of41 files yields ten declared hooks and zero other differences.
User confirms menu looks good in PID768; wide race capture also viewed.
The transition into loading is reported rough and remains a separate open
visual verification item.


### Loading handoff coherence

User screenshot shows blue side panels only over the updated middle100 rows,
with black corners above and below. State capture records a42-frame temporary
wide interval (10614..10656) before returning4:3, then real world rendering at
11057. Menu projections had refreshed GTE history even while explicit UI mode
was active. Releasing UI mode reused this stale classification until expiry.
Meanwhile GP0 full blue clears during UI mode had never reached wide buffers,
as that mirror was gated on active wide presentation.

The optional solid-background policy now also maintains full-width clear
coherence in hidden wide surfaces. It configures the known aspect extent and
mirrors the same clear rows/color; smaller texture fills do not enter this new
path. Leaving explicit UI mode expires old projection/tag detection history;
fresh game geometry rearms as usual. RelWithDebInfo build succeeds; candidate
PID23472 transition accepted by user: looks proper now.
