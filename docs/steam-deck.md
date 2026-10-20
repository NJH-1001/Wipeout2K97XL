# Steam Deck AppImage and Steam Controller (2026)

Download `wipeoutxl-0.1.1-steamdeck-x86_64.AppImage` and its SHA-256 checksum
from the [main release](https://github.com/NJH-1001/Wipeout2K97XL/releases/tag/v0.1.1).
This is a Linux x86-64 setup AppImage for Steam Deck and compatible desktops.
Supply your own Wipeout XL USA SCUS-94351 CUE and all 12 BIN tracks.
The image contains no game data or generated game code.

## Install and play

1. In Steam Deck Desktop Mode, save the AppImage in a permanent location,
   such as `~/Applications/`. In file Properties, allow executing it as a program
   (or run `chmod +x wipeoutxl-0.1.1-steamdeck-x86_64.AppImage`).
2. Add the AppImage as a **Non-Steam Game** in Steam. Use native Linux launch;
   do not force Proton for this artifact. If mounting fails because FUSE is
   unavailable, add `--appimage-extract-and-run` to its launch options.
3. Launch it from Steam in Desktop Mode for initial setup. Keep OpenBIOS
   selected, choose your CUE, and use **Generate & rebuild**. Let the wizard
   obtain its portable compiler/toolchain. Python is already bundled.
   Initial setup requires Internet access and space for the unpacked kit,
   compiler, your disc and local build. It does not require modifying SteamOS's
   system packages or disabling its read-only protection.
4. Reopen the same AppImage when the build completes. It forwards to your
   locally built player. Keep this same Non-Steam shortcut for Gaming Mode.
5. In Steam's controller settings for that shortcut, enable **Steam Input**
   and choose a **Gamepad** layout. Select the gamepad as Player 1 if the
   launcher does not select it automatically.

The same shortcut supports Deck's built-in controls and a paired wireless
**Steam Controller (2026)**. Pair the latter through Steam's controller settings;
see Valve's [2026 controller guide](https://help.steampowered.com/en/faqs/view/33E8-5EDF-24E6-4CFB).
The route implemented here is **Steam Input gamepad emulation**, which provides
standard gamepad axes/buttons to SDL3. It is not a Steam Input API/action-manifest
integration or a claim of direct wireless-driver validation.

## Enhanced NeGcon layout

| Steam Input output | In-game action |
| --- | --- |
| Left joystick horizontal | Analog steering |
| Left joystick vertical | Unassigned |
| Right joystick vertical | Nose pitch |
| Right joystick left / right | Right / left air-brake (reversed) |
| Left / right bumper | Left / right air-brake |
| Right trigger **analog axis** | Analog acceleration |
| Left trigger | Use equipped weapon |
| A (bottom face) | Full acceleration / menu select |
| X (left face) | Use equipped weapon |
| B (right face) | Discard weapon |
| Y (top face) | Change view / menu back |
| D-pad / Menu | Navigation / pause |

Keep the sticks as joystick outputs and the right trigger as an analog trigger;
mapping them to keyboard keys would lose analog steering/throttle. Rear buttons
may duplicate bumpers or other existing buttons through Steam Input. Extra
touch/gyro sensors are not consumed directly by the game. The existing
**NeGcon enhanced dual sticks** mod exposes individual assignment choices.
Pitch and air-brakes retain the game's digital behavior.

Steam Input is the default AppImage mode. It suppresses the duplicate direct
Deck/legacy-Steam HIDAPI path and allows SDL's Steam virtual gamepad path.
For advanced direct Deck input with Steam Input disabled, use
`WXL_STEAM_INPUT_MODE=direct-deck` in the launch environment. That mode does
not claim direct support for the 2026 wireless controller. Explicit SDL
environment overrides are respected.

## Local data and updates

The read-only image seeds a writable copy under
`~/.local/share/Wipeout2K97XL/versions/<payload-id>/` (or `$XDG_DATA_HOME`).
Disc preparation, game generation, compiler outputs and rebuilt player live
there. The AppImage never modifies itself or incorporates the user's data.
Saves, input bindings, launcher settings and mod selections are kept under
`~/.local/share/Wipeout2K97XL/profile/`, including across changed image payloads.
Saves are shared immediately; settings are synchronized when the launcher/player
closes, including settings saved by atomic file replacement. One instance may
use a profile at a time. Keep this directory backed up. A new payload uses a separate build directory;
it may require disc selection and rebuilding again.

`WXL_APPIMAGE_DATA_DIR` can specify another **absolute** writable data directory.
Dependencies and bundled Python are copied there so a background rebuild can
continue after the original launcher exits and the image unmounts.

## Verification

The image is built from the checksum-pinned released Linux kit. Its setup
executable and emitters are retained byte-for-byte. Packaging checks native ELF
architecture/dependencies and forbidden content, starts portable Python with
HTTPS, tests persistent profile/update behavior, and opens the actual AppImage's
launcher under a virtual Linux desktop. A screenshot is reviewed before release.

**Steam Deck hardware, Gaming Mode, a generated game race, the 2026 wireless
Steam Controller and its physical analog inputs remain unverified.** This is a
preview with the same native gameplay verification limits as the Linux kit;
it has not received Valve's Steam Deck Verified certification.

Sources: [Valve gamepad-emulation documentation](https://partner.steamgames.com/doc/features/steam_controller/steam_input_gamepad_emulation_bestpractices),
[SDL3 gamepad support](https://wiki.libsdl.org/SDL3/CategoryGamepad),
[SDL Deck HIDAPI hint](https://wiki.libsdl.org/SDL3/SDL_HINT_JOYSTICK_HIDAPI_STEAMDECK),
[AppImage packaging documentation](https://docs.appimage.org/packaging-guide/overview.html).
