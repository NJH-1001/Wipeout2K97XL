# Steam Deck and Steam Controller

Use the **Linux x64 setup ZIP** from the
[main release](https://github.com/NJH-1001/Wipeout2K97XL/releases/latest).
The owner reports this version working on Steam Deck. Dedicated Steam Deck
AppImages have been withdrawn after setup/play failures; use the Linux ZIP
instead of an older AppImage download.

## Install and play

1. In Desktop Mode, extract the complete Linux x64 ZIP into a permanent,
   writable folder. Keep its files together.
2. Follow the [Linux prerequisites and setup instructions](native-platforms.md),
   including Python 3.11 or newer. Run `Wipeout2K97XL` from that folder.
3. Supply your own Wipeout XL USA SCUS-94351 CUE and all 12 BIN tracks. Keep
   OpenBIOS selected and use **Generate & rebuild**. Allow it to finish.
4. Reopen the same setup executable, which forwards to the locally built player.
5. Add `Wipeout2K97XL` as a Non-Steam Game and use native Linux launch.
   Enable Steam Input and select a Gamepad layout for Deck controls or the
   paired wireless Steam Controller (2026). Select the gamepad as Player 1
   in the launcher if necessary.

The controller route is Steam Input gamepad emulation through SDL3. This does
not claim direct wireless-driver or Steam Input API/action-manifest integration.
Keep stick outputs as joystick axes and the right trigger as an analog trigger.

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

## Existing AppImage users

Removing the download does not remove your local files. Before changing setups,
back up any saves and settings under
`~/.local/share/Wipeout2K97XL/profile/` and your previous writable project under
`~/.local/share/Wipeout2K97XL/versions/`. Keep those backups until the Linux kit
is configured and your saves are confirmed. Do not add AppImage-specific launch
arguments to the Linux executable.

## Verification

Linux build, dependency and launcher checks pass in CI. The owner reports the
Linux ZIP working on Steam Deck. Detailed Gaming Mode, save/reload, display-mod
and 2026 wireless Steam Controller testing remains unverified. No Valve Steam
Deck Verified certification is claimed. Mac gameplay remains unverified.
