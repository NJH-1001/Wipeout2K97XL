# Draw-distance findings and proposed experiment

The baseline has no draw-distance modification. An experimental plugin is now under test. Widescreen remains
separate and the user reports it looks good.

## Verified limits

The whole-quad track renderer (0x80013C78) rejects camera-relative components
outside -32768..32767 before packing GTE inputs. Its depth-based packet
insertion feeds an 8192-entry ordering table. The background model uses
fixed bucket 8191: both Ghidra and a native call with a2=0x1FFF confirm this.
Removing coordinate/depth checks without redesigning ordering is unsafe.

Track geometry is selected through WAD-supplied section/run pairs, installed
by 0x8001F240. Scene objects use a separate candidate builder at 0x80046594,
with section radius fields and a 128-entry temporary array. The object far
cutoffs are 20000..38200 units depending on track/mode. Increasing one cutoff
does not extend the whole pipeline.

A captured 349-section track already uses radius 32767 everywhere and its
all-direction lists contain nearly every section within a 30000-unit cube.
In a separate Beetle track-index-4 camera snapshot, an offline projection
classifies 202 faces inside the projected bounding rectangle as outside the
signed input range. This does not establish that all 202 would be visible:
occlusion, GTE output flags and actual directional-list selection are not
included in that statistic.

## Concrete proposed prototype

A separate, default-off **Extended Track Distance** experiment would:

1. Read the already-loaded track vertices, faces and verified root camera
   transform. Leave physics and the original near renderer unchanged.
2. Project only additional faces outside the original representable range,
   initially bounded to camera depth 64000. Use expanded intermediate math.
3. Reuse the documented original texture/UV layout and construct additional
   packets in separately allocated enhancement memory. Keep two frame buffers.
4. Sort additional packets by depth and place them after the fixed-depth
   background, preserving the original ordering-table allocation bounds.
5. Expose diagnostic counts, capacity failures and unsupported camera cases
   through the TCP-readable enhancement memory, and remain off by default.

This first experiment would extend **road geometry only**. It would not fix
scenery, billboard or effect pop-in. It could show incorrect occlusion or
seams until projection and draw-order tests pass; it must not be described
as a verified draw-distance fix or included as an enabled release feature.

Required checks before promotion: original-range projection comparison,
packet and buffer bounds, background/near/far ordering, before/after views
at the same camera, multiple tracks/cameras, and unchanged behavior with
the feature disabled. Scenery extension requires its own analysis.

## Experiment authorization

Automatic approval review rejected writing and integrating the prototype
because it is an unverified per-game rendering change, conflicting with the
user's original no-unverified-hacks instruction. The rejected command did
not execute. No prototype source, package or configuration hook was added.
The user subsequently replied "yes, please continue", explicitly authorizing this default-off experiment. Implementation and testing are now in progress; this does not promote it to a verified release feature.

The independent Beetle tool failure encountered during capture is fixed:
reset client sockets no longer retain its sole connection slot. Four forced
reset/reconnect checks passed. Evidence hashes are recorded in
`verification/draw-distance-manifest.json`; all raw game data remains ignored.

## Prototype implementation and verification (2026-09-07)

The authorized experiment now lives in `src/track_distance_mod.c`, selected by
`wxl.presentation.track-distance` / `distance`. It remains default-disabled in
the bundled catalog; the local test build enables it alongside widescreen.
It reads resident road faces and adds only faces with a camera-relative vertex
outside the original signed input range. All four projected vertices must have
depth 160..64000 and fit the GPU screen-coordinate range. This conservative
prototype rejects crossing faces instead of wrapping coordinates. It does not
extend scenery, billboards, effects, or list-only omissions within the original
input range.

Only root camera transforms and H=160, center=(160,120), 320x240 are supported.
Unknown projection/camera states skip the added pass and report a status.
The 8192-entry original OT is preserved; added depths clamp to bucket 8190,
below the verified fixed-background bucket 8191. Two 327680-byte packet buffers
are allocated from the enhancement DMA aperture. No game heap is enlarged.

Checks so far:
- Clang synthetic tests pass projection limits, disabled identity, error
  atomicity, original FT4 texture/page/color layout, background-slot preservation,
  and a full-capacity 8192-packet chain without writes beyond its buffer.
- The framework's DMA-aperture address/boundary tests pass.
- Regenerating without the mod entry configuration and comparing all 41 C files
  proves only four callback calls differ. No guest instruction or dispatch entry
  changed, and no generated source was hand-edited.
- A disabled native launch reads open bus from the unallocated diagnostics area.
- OpenBIOS live track-index-0 samples add 95 and 105 polygons; the latter buffer
  passes a coherent readback of every packet, coordinate and link.
- A 41-sample ordinary-range comparison to native GTE trace has maximum one-pixel
  error (exact division versus hardware reciprocal approximation). This is not
  independent Beetle projection parity and may cause boundary seams.
- Enabled/disabled full-window captures have equal camera transforms. Animated
  content differs between frames, so they are not pixel-identical scene replay.
  Nearby geometry is visible; this indoor view does not prove open-road distance
  improvement or complete occlusion correctness.

The prototype remains under verification: multiple tracks, open-road views,
seams, overpasses/occlusion and physical driving feedback are outstanding. It is
not included in an updated release package.

### Bounded diagnostics

This build allocates 64 bytes at 0x9F000000 (verify magic `0x444C5857` first).
The sixteen words are: magic, version, pass count, status, last packet count,
cumulative packet count, DMA arena, camera node, diagnostic enable (0/1),
capacity, rejected projection count, original-input-range count, outside-viewport
count, track index, OT base and buffer index. Status 0 means completed; 1 world
validation, 2 camera hierarchy, 3 projection/display, 4 allocation/OT, 5 vertex
index, 6 conflicting cameras, 7 diagnostic disable. `0xFFFFFFFF` means an update
is in progress. The live probe rejects changing samples and restores its enable
word after A/B captures. Use `tools/verify_track_distance_live.py` at TCP 4370;
all raw evidence stays under ignored analysis/ and captures/ directories.

### Scenery follow-up evidence

The main object renderer at 0x80010000 stores per-vertex flags as
`(FLAG >> 12) & 0x466`, including IR3 saturation. Its triangle/quad paths use
AVSZ3/AVSZ4, while billboard paths use a vertex depth byte offset. Therefore
raising the object far tables alone is insufficient: projection eligibility,
primitive variants, ordering and arena capacity need their own verified design.
This observation comes from the cached Ghidra disassembly/decompilation; no
scenery code or cutoff was changed.

Final candidate check: Windows x86-64 RelWithDebInfo, OpenBIOS LLE. The projection
guard passes in the snowy demo. A subsequent bounded track-index-0 sample advances
484 road passes with only successful statuses and up to 311 additional polygons;
four complete coherent packet buffers pass validation. The local candidate is
left running with the experiment enabled for driving feedback. Catalog defaults
and the previously delivered release package are unchanged.
