# Frame-rate investigation — 2026-10-22

With smoothing disabled, the Windows build presents at approximately 60 Hz but submits new race
frames at approximately **30 fps**. A bounded five-second Talon race sample
measured 29.95 submissions/s and 59.71 VBlanks/s; almost all sampled submissions
were two refreshes apart. Presentation frequency alone is not game frame rate.

Ghidra identifies the pacing routine at `0x800684C4`, including a conditional
second VSync wait at `0x80068550..0x80068560`. All 548 bytes of this routine match
the legally owned executable in both native and independently booted Beetle RAM.
Beetle also shows two-refresh submission gaps in credits; a matched race sample
remains outstanding. Raw captures and game-derived listings stay local.

The elapsed-refresh byte is `0x800947A4`, but it is not a universal physics delta.
Player update `0x80024878` includes per-call steering and countdown adjustments;
position integration `0x80030154` and effect spawning `0x80046F00` also contain
per-call operations. Removing the frame wait alone is not a verified 60 fps fix.

The owner selected original simulation cadence with intermediate geometric
rendering. The development implementation adds an optional, default-off
**Smooth rendering** package in Mods → Visual. It requires OpenGL.
It draws intermediate polygon positions into separate color/stencil surfaces.
The 60/90/120 fps choices use one, two or three intermediate phases per source
frame. The same verified object renderer supports menu previews. It does not blend completed images,
remove the original frame waits, advance physics twice, or change input polling,
audio or timers. HUD/effect updates retain their original cadence. The midpoint
schedule introduces one display refresh of visual delay, approximately 16.7 ms
at 60 Hz; practical latency and racing feel still require validation.

Read-only metadata callbacks follow 41 exact-word-verified stores. Generation
tests establish that all 40 game shards are identical after removing only these
callbacks, dispatch is identical, a wrong instruction is rejected, and an empty
hook configuration reproduces the original output. Source words match the legal
executable; hook evidence also includes independent native/Beetle RAM checks.
Temporary packet reuse is guarded by the complete GP0 payload. Prior-pose GTE
projections must reproduce the current polygon coordinates before history is
accepted. Missing/ambiguous history, scene changes and unsupported framebuffer
copies fall back to original positions or frames. Neither intermediate surfaces
nor projection history are guest VRAM or saved game state.

The Windows RelWithDebInfo/OpenBIOS development build passed a bounded moving
race test: **29.95 source frames/s and 60.10 displayed frames/s**, split between
original and geometrically intermediate frames. A paired capture of the same
source frame showed 419,276 changed pixels. Road subdivisions, scenery,
billboards, distant opponents and exhaust carry guarded position history;
3,303 sampled billboard projections all reproduced the original corners before
history was accepted. Unsupported or discontinuous geometry falls back safely.
This is projected-polygon interpolation, rather than a second simulation step.

Packet-reuse and projection-purity tests pass. The Mods catalog test verifies
the default-off Visual checkbox, persisted selection and trusted plugin
activation. A running OpenBIOS build also activates from the saved Mods selection
with both developer activation switches disabled. A separate coexistence test
with legacy frame blending requested measured 29.90 source and 59.81 displayed
frames/s; the old blender produced no swaps while geometric smoothing ran.
The curated framework patch reproduces the implementation from
clean LF and CRLF checkouts. All 41 hook instruction words also match the legal
executable and both independently running guests.

Physical racing feel and input-to-photon latency remain unverified. Exhaustive
track/effect coverage and Linux/macOS runtime checks also remain outstanding.
These are development measurements. The v0.2.0 setup kits are built and audited
separately on native CI runners; see the release verification manifest. Private
evidence stays in `analysis/framerate/`.

## v0.2.0 candidate checks (2026-10-22)

The owner completed a race and accepted the smoother appearance. The final
Windows candidate measured menu presentation at 59.70, 90.17 and 119.83 fps
for the 60/90/120 choices, with source updates near 30 Hz and VBlanks near
60 Hz. Race samples measured 59.77 and 89.92 fps at the two lower choices.
The initial 120 fps race sample delivered 112.71 fps with 28.28 source updates/s
and 56.55 VBlanks/s: the extra rendering can exceed the host's frame budget.
These are measured rendering rates, not guaranteed performance on every GPU.

The original/intermediate frame pairs contain distinct moving geometry. Tests
cover schedule phase ownership, reused packet payload guards, disabled identity,
consecutive-frame distant-object history, catalog defaults and persistent
60/90/120 selection. Regeneration verifies 41 exact hook instructions, 40 guest
shards unchanged after removing metadata callbacks, unchanged dispatch, rejection
of an incorrect source instruction, and identical output with hooks disabled.
The curated framework patch applies to both LF and CRLF clean pinned sources.
Viewport-edge culling includes both current and previous positions; original
raster clipping remains authoritative. Full track/camera coverage is still open.
