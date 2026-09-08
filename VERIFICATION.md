# Release verification

## Confirmed development behavior

OpenBIOS boots the game; keyboard, PowerA wired Switch 2 and Xbox controls were
accepted in user race tests. Xbox proportional acceleration and NeGcon steering
were independently sampled. User accepted wider scene bounds, extended scenery
and pipes, distant ships and corrected exhaust width, weapon-hit draw distance,
victory background and loading/menu transitions. Reversed right-stick brakes
and right-stick pitch were user approved.

The new configurable controller policy passes 729 neutral/shoulder option
combinations plus directed approved-preset, half-pressure and disabled-input
checks. The setup host builds as Windows AMD64 RelWithDebInfo. Its mod catalog
contains four title packages, including NeGcon enhanced dual sticks.

## Setup-kit checks

The Windows ZIP passes CRC, required-file and forbidden-content checks. Its
bundled tools generated OpenBIOS and game functions from the legal 12-track
CUE, then compiled a full playable executable from the extracted source tree.
The user confirms the new mod dropdowns are visible and usable. The actual
catalog's defaults, option validation, saved choices and disable/reload behavior
pass automated checks. The user confirms the extracted build boots and the approved dual-stick
controls still work correctly in a race. The setup host is RelWithDebInfo;
the player wizard builds its playable executable in Release configuration.

Full-game completion, exhaustive 3D rendering-path analysis, every track/camera,
save/load round trips, separate Windows 10 and 11 systems, AMD hardware and
untested controller families remain outside the completed verification scope.
Raw diagnostic evidence stays local.


## Independent Windows release build

GitHub Actions run 34230697278 passes compilation, packaging and audit. The
public kit uses that clean-runner artifact, with seven standard runtime license
notices added. All 2121 original entries were independently compared byte for
byte with the downloaded Actions artifact. The runner had no private disc data.
All nine packaged PE files pass an explicit-filename dependency audit. The
setup launcher opens with PATH restricted to Windows system directories.
Its bundled emitters generate the same 41 game C files as the previously
compiled, user-tested build; generated files remain local and outside the ZIP.

An earlier local ZIP was withdrawn after discovering omitted LLVM runtime DLLs.
The corrected published kit is the CI-produced archive described above.
