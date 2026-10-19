# Third-party components

Components retain their upstream licenses; included license files are authoritative.

- PSXRecomp: https://github.com/mstan/psxrecomp (bundled framework sources).
- recomp-ui: https://github.com/mstan/recomp-ui (bundled launcher sources).
- OpenBIOS / PCSX-Redux: https://github.com/grumpycoders/pcsx-redux;
  see psxrecomp/bios/OpenBIOS.LICENSE and bios/patches for local allocator changes.
- SDL3: https://github.com/libsdl-org/SDL (zlib license).
- SDL_GameControllerDB: https://github.com/mdqinc/SDL_GameControllerDB;
  see docs/controller-mapping-LICENSE.txt.
- MiSTer display preset references: https://github.com/MiSTer-devel/Presets_MiSTer.
  CRT effects are original approximations, not copied game assets.
- Additional bundled SDK dependencies retain the license notices accompanying
  their source trees and archives. MinGW runtime DLLs retain their upstream terms.

No rights to the original game are granted. This compatibility project does not
include the original game's content or retail Sony BIOS firmware.

Runtime DLL notices are included under docs/licenses (LLVM, GCC runtime
exception, winpthreads and SDL3).

AppImage-only components: [AppImage type2-runtime](https://github.com/AppImage/type2-runtime),
[python-build-standalone](https://github.com/astral-sh/python-build-standalone)
(CPython 3.12.15, release 20261013, with included license files), Ubuntu runtime
libraries and CA certificates (copyright notices copied into
`docs/licenses/appimage` inside the payload). The build uses
[appimagetool](https://github.com/AppImage/appimagetool). Download hashes are pinned
in `packaging/appimage/dependencies.json`; native GPU drivers and glibc are supplied
by the host system. Steam Input is supplied by the user’s Steam installation;
no Steamworks SDK or Valve proprietary controller driver is bundled.
