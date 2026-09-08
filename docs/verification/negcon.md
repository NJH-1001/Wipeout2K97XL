# NeGcon verification

Status: independent oracle and game packet decoding verified; native SIO,
SDL steering mapping and on-track behavior are not implemented/verified yet.
The user-confirmed native digital gamepad remains the baseline.

## Independent input and wire evidence

`beetle_libretro.cpp` now provides bounded controller inputs through
`controller_input`: mode digital/negcon, twist -32767..32767, and i/ii/l
pressures 0..32767. Commands run between retro_run calls. This selects Beetle's
existing libretro NeGcon device and supplies input callbacks; it does not
modify guest memory or substitute game code. Invalid modes/ranges fail.
All fields default to neutral/released when omitted. Digital button presses
remain controlled by the existing press/set_input commands.

The independent implementation is
`psxrecomp/beetle-psx/mednafen/psx/input/negcon.cpp`, with frontend conversion
in `input.cpp`. Its poll ID is 0x23; response after initial high impedance is
ID, 0x5A, two active-low button bytes, twist, I, II, L. Neutral twist is 0x80;
pressure zero means released. This is distinct from DualShock 0x73 with four
stick axes. No Beetle implementation source was copied into native SIO.

`python tools/verify_negcon_oracle.py --port 4382` verifies five directed
cases against live SIO captures: neutral; left with I pressure; right with II
pressure; L pressure; and distinct partial values on all three pressure axes.
Each case requires at least two complete matching packets. All passed.
Invalid mode and out-of-range values were rejected; switching to digital
restored 0x41 on port 1. The raw artifacts remain ignored and local.

## Game analysis through Ghidra MCP

Ghidra is connected to SCUS_943.51.text.bin, payload hash recorded in the
manifest. Function 0x80065234 accepts packet headers 0x4100 and 0x2300.
Other headers take the no-controller branch. The second argument is a decoded
record: +0 type (1 digital, 2 NeGcon), +2 current buttons, +4 newly pressed
buttons, +6 previous buttons, +8 selected analog action pressure, +9 twist.
For NeGcon it copies input byte +4 into decoded byte +9. Bindings at or above
0x8FF0 select analog pressure channels, with a 0x80 threshold for digital
button interpretation; an analog action channel is preserved separately.
Function 0x800657CC supplies menu button state and controller type at
0x80094B3C. Its NeGcon path thresholds the three pressure bytes separately.

Live oracle calls use raw packet 0x80094CBC and decoded record 0x800F1DDC.
Readback verified type 2 and decoded twist values 0, 128 and 255 after the
three corresponding TCP axis inputs. This establishes input transmission
and game decoding, not correct ship response or calibrated sensitivity.

Twist consumers include 0x8003D158 (idle detection), 0x800747A0 (recording:
NeGcon stores a second word containing twist/analog action), and 0x8004C0E4
(record exchange). Ship processing at 0x80023D2C passes the decoded input
record as the fifth argument through a callback at ship record +0xEC, with
ship stride 0xF0. Complete callback classification, calibration, steering
scales and live trajectory verification remain open. These descriptive
roles are supported by data flow; they are not recovered original symbols.

## Next verification gate

Implement a distinct native NeGcon device coherently across serial polls,
unsupported commands, ACK scheduling, mode changes, snapshots and host input.
Verify its packets against the independent captures and test transaction
boundaries. Then map SDL left-stick X to twist and test menu navigation,
calibration, racing, replay and save/load. Do not enable DualShock analog or
claim that digital stick thresholds provide NeGcon steering.


## Native transaction model and SIO integration blocker

The standalone native `negcon_protocol.h` byte-boundary model and
`negcon_protocol_test` now pass the five captured packet cases, latched-input
checks, unsupported-command behavior, deselection, transaction continuation
from copied device state, and ACK delay checks against Beetle source (256
clocks after address/command, 128 after intermediate response bytes, no ACK
after the final byte). This is not yet connected to SIO or SDL, and copying
the isolated device state does not establish full-runtime snapshot support.

Before integration, the disabled full-SIO DualShock rumble test was built and
run. It links successfully now, but fails 26 checks. A local diagnostic copy
changes only its `sio_tick(2000)` call into 125 calls to `sio_tick(16)`; all
checks then pass. Both advance the same nominal 2000 guest cycles. The
production `sio_pace_walk` permits only one transition per call and loses the
remaining elapsed cycles on exit. This proves grouping-dependent behavior;
it is not a controller-mapping error. The existing test remains unmodified
and disabled; replacing its timing call would hide the production defect.

Per the user's stop-on-verification-blocker instruction, native integration
is stopped at this gate. The next correction must make the shared scheduler
account for every elapsed cycle and verify ACK/IRQ ordering against Beetle,
including card traffic and independent boot/game regressions. Removing the
limit without those checks would be an unverified peripheral change.
Evidence hashes and outcomes: `sio-timing-blocker.json`. The working native
game executable and digital controller configuration were not changed.


## Elapsed-cycle correction verified (continuation)

The shared scheduler now processes all due shift/ACK events and consumes
remaining cycles in deadline order. The unchanged 2000-cycle rumble sequence
passes, along with 1/16/127-cycle variants, receive-ready/ACK boundary checks,
card protocol and SIO/SPU interrupt-ownership tests. The prior disabled rumble
test is re-enabled. This supersedes the lost-elapsed-time integration blocker.

The rebuilt native executable and independent Beetle produce matching complete
digital Start packets (FF 41 5A F7 FF) through their actual SIO traces. This
supports serial behavior, not exact per-device ACK latency or full game parity.
The existing 170-clock ACK default was not changed by this correction; NeGcon
requires its distinct device delays when integrated. Other-title regressions
and full native NeGcon integration remain open.

The scheduler-corrected native build visibly reaches post-intro 3D credits.
The captured live status reports 146 completed card reads, zero card aborts
and zero dirty-code aborts. Racing, native NeGcon and saved-game operations
are not established by this boot check.

## Native integration, 2026-09-07

The native SIO bus now supports a distinct NeGcon device. The command latches
buttons and twist/I/II/L pressure bytes; ACK deadlines are 256 CPU cycles after
the address and command, 128 after intermediate responses, and absent after the
last byte. DTR release/reassertion is required after completion or rejection.
The saved bus phase, response bytes and pending deadline preserve polls across
snapshot restoration. ACK telemetry was widened to 16 bits; both historical
8-bit metadata snapshot variants remain readable.

Twelve SIO/launcher CTests and the launcher wiring guard pass. Full-SIO NeGcon
tests cover every response byte with a pending-ACK snapshot, multiple cycle
step sizes, unsupported addresses/commands and digital restoration. Five live
native input cases match the independently captured Beetle packets using the
same TCP verifier. Native `sio_trace_reset` now resets visible history without
invalidating monotonic byte IDs used by other rings.

`default_mode = "negcon"` is supported. The ignored local candidate uses it;
the tracked game configuration remains digital until the on-track check.
SDL left X drives twist through a one-dimensional rescaled deadzone. Cross or
right trigger supplies I, Square or left trigger supplies II, and L1 supplies
L. Triggers preserve proportional pressure; face/shoulder buttons supply full
pressure. The original wired digital button positions are retained. D-pad
navigation remains independent of twist. Keyboard A/D produces endpoint twist;
its existing Cross/Square/L1 bindings supply the pressure controls.

The native TCP `controller_input` uses the same normalized input units as
Beetle, but requires the selected mode to match the configured native device;
it does not replace a controller during an in-flight poll. `clear_input`
restores physical sampling. NeGcon is currently direct-port/offline support;
multitap and netplay are not verified by these tests.

Ghidra traced the player callback transition from 0x80024474 to 0x80024878.
The latter consumes decoded twist at input+9 when decoded flags+2 has bit 2.
It subtracts calibration center 0x80094210, applies a deadzone selected through
0x8009420C from the halfword table at 0x80093E8C, then scales by ship+0xA2,
the divisor at 0x80093E16, and slews ship+0x76 using ship+0xA0. The game thus
already implements a proportional steering path and its own rate limiting.
This is static analysis, not yet an on-track native/Beetle trajectory proof.
No game steering instructions or calibration RAM were patched.

### Physical steering confirmation

The user confirmed on-track PowerA Switch 2 steering is working very smoothly.
The captured native race display was visually verified. `game.toml` now uses
`default_mode = "negcon"`, `lock_mode = true`, `allow_hybrid = false`.
The Xbox analog-trigger test is pending; Switch-family trigger travel must not
be treated as proportional pressure without a hardware reading.

### Xbox confirmation

SDL switched P1 to Microsoft controller GUID
`0300fa675e040000ff02000000007801`. A bounded 35.21-second passive TCP sample
observed NeGcon I pressure 0 and 255 plus 83 distinct intermediate values.
The user confirmed that light versus full trigger pressure changes acceleration
and that steering and the remaining controls work. Raw packet samples remain
ignored in `analysis/verification/xbox-negcon-trigger.json`.
