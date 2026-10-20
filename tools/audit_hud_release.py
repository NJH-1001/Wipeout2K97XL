"""Ensure a release contains the exact HUD implementation and mod catalog."""
import pathlib
import sys
import zipfile

root = pathlib.Path(__file__).resolve().parents[1]
with zipfile.ZipFile(sys.argv[1]) as archive:
    files = ["src/widescreen_mod.c", "game.toml", "VERSION",
             "psxrecomp/runtime/include/ws_hud_packet.h",
             "psxrecomp/runtime/include/gpu.h", "psxrecomp/runtime/src/gpu.c"]
    for name in files:
        assert archive.read(name).decode().replace("\r\n", "\n") == (root / name).read_text(encoding="utf-8"), name
    package = "wxl.presentation.widescreen/1.0.0/manifest.toml"
    expected = (root / "mods/preloaded/packages" / package).read_text(encoding="utf-8")
    for prefix in ["mods/bundled/", "mods/preloaded/packages/"]:
        assert archive.read(prefix + package).decode().replace("\r\n", "\n") == expected, prefix
print("PASS: exact HUD source, renderer, game configuration, version and both mod catalogs")
