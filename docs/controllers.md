# Controller verification

SDL3 3.4.10 is linked into the Windows runtime. Its standard positional gamepad
API provides the mapping path for Switch, Switch 2, PS4, PS5 and Xbox families.
The PowerA Switch 2 and Xbox controllers have been tested in-game; PS4, PS5
and other Switch hardware have not been physically tested here.

Attached device: PowerA Advantage Wired Controller for Nintendo Switch 2,
VID 0x20D6 / PID 0xA720, SDL GUID 03007bbfd620000020a7000000000000.
The installed SDL database did not recognize it. The exact published Windows
mapping from [SDL_GameControllerDB](https://github.com/gabomdq/SDL_GameControllerDB/blob/55c3bc818853e77937c2373527533d7baecd42f8/gamecontrollerdb.txt)
now makes SDL recognize, open and report it connected. The database's zero-CRC
GUID matches this device through SDL's own matching; no guessed button indices
or device-specific runtime bypass were added. The mapping's zlib license is
included and staged with it.

`tools/controller_probe` is a bounded SDL3 inventory tool linked against the
same installed library. Run `controller_probe.exe gamecontrollerdb.txt` to
verify recognition. The runtime now loads the staged database before opening
player devices. Initial live testing found keyboard worked but controller did
not: Player 1 had defaulted to keyboard. `[controller] p1_device = "auto"` now
selects the first recognized gamepad. On 2026-09-07 the user confirmed that
the attached gamepad worked in-game after this routing change. The launcher
can select Keyboard instead; routing is exclusive in the current framework.

`keybinds.ini` supplies WASD plus arrows, Space/X for Cross, Shift/Z for Square,
C for Triangle, Ctrl for Circle, Q/E for L1/R1, F/R for L2/R2, Enter for Start,
and Backspace for Select. Names are PSX buttons; in-game assignments remain
authoritative. Defaults are staged only if absent, preserving user rebindings.

The current default is locked NeGcon mode with hybrid mode disabled. The user
confirmed smooth analog steering on PowerA Switch 2 and Xbox, and proportional
right-trigger acceleration on Xbox. See [NeGcon verification](verification/negcon.md).

## Current stick mapping (2026-09-08)

| Control | Action |
| --- | --- |
| Left stick left/right | Analog NeGcon steering |
| Left stick up/down | Unassigned |
| Right stick up/down | Pitch up/down through original directional inputs |
| Right stick left/right | Right/left air-brake (reversed), alongside L1/R1 |
| L1/R1 | Left/right air-brake, retained |
| Right trigger | Analog acceleration on controllers with analog triggers |
| D-pad | Original directional inputs, retained |

The right-stick brake mapping currently uses the original on/off brake response.
Ghidra analysis and independent Beetle code comparison show that SCUS-94351's
NeGcon decoder (0x80065234) thresholds brake pressure at 128. Its brake routine
(0x80065B0C) consumes only action bits and ramps the two brake states by 32 per
update, clamped to 0..256. Only throttle pressure and twist survive as continuous
inputs in the decoded input record. Proportional right-stick air-braking therefore
requires a separately verified gameplay modification; it is not implemented by
this mapping change.

The RelWithDebInfo build and existing controller routing guard pass. The user confirmed the initial
physical right-stick mapping works well, then requested reversed right-stick
brakes for comparison. Proportional brake enhancement is deferred at their request. Existing installations
preserve input.ini; the development build's copy has been updated explicitly.


## Public release requirement: NeGcon enhanced dual sticks

User request, 2026-09-08: expose the controller enhancements together under a
mod named **NeGcon enhanced dual sticks**, with independently configurable
sub-options. The user confirmed the reversed right-stick brakes feel good.
Implemented in the public-release candidate: eight launcher options, a recommended
preset and input.ini bypass. User confirms the dropdowns are usable. The actual
catalog passes defaults, validation, persistence and disable/reload tests.

Required settings must cover all requested controller changes:

- Analog steering assignment (current preset: left-stick horizontal).
- Pitch assignment: left-stick vertical, right-stick vertical, or disabled;
  current preset uses right-stick vertical and leaves left-stick vertical free.
- Right-stick horizontal air-brakes: enabled/disabled and normal/reversed
  directions; current preset is enabled and reversed (left activates right
  brake, right activates left brake).
- Independent shoulder air-brake bindings; current preset retains L1/R1.
- Analog acceleration assignment and enable/disable; current preset uses the
  right trigger, with the existing face-button acceleration alternative.
- Preserve editable face-button, D-pad, menu and keyboard bindings, and existing
  supported controller-family mappings. Do not overwrite user customizations.

Provide a named default preset matching the user-approved current scheme,
persist individual choices, and provide reset-to-preset. Options should clearly
identify controls that remain digital (pitch and air-brakes); do not advertise
analog brake pressure. The user declined a separate analog brake gameplay mod.

Before public release, verify option combinations, settings persistence and
mod enable/disable behavior, and physically retest the current preset on the
available Xbox and PowerA Switch 2 controllers. Confirm package defaults include
the approved preset and upgrades preserve existing custom mappings.
