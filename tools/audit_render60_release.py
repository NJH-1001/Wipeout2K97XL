"""Require the release kit to contain the exact reviewed smoothing sources."""
from pathlib import Path
import sys
import tomllib
import zipfile

root = Path(__file__).resolve().parents[1]
files = ["src/render60_mod.c", "src/track_distance_experimental.c", "game.toml",
         "VERSION", "psxrecomp/runtime/src/gpu.c",
         "psxrecomp/runtime/src/gpu_gl_renderer.c",
         "psxrecomp/runtime/src/geometry_gl.inc",
         "psxrecomp/runtime/include/geometry_packet_tag.h",
         "psxrecomp/runtime/include/geometry_projection.h",
         "psxrecomp/runtime/include/geometry_schedule.h"]
with zipfile.ZipFile(sys.argv[1]) as archive:
    for name in files:
        assert archive.read(name).decode("utf-8").replace("\r\n", "\n") == (
            root / name).read_text(encoding="utf-8"), name
    package = "wxl.presentation.render60/1.0.0/manifest.toml"
    expected = (root / "mods/preloaded/packages" / package).read_text(encoding="utf-8")
    for prefix in ("mods/bundled/", "mods/preloaded/packages/"):
        assert archive.read(prefix + package).decode("utf-8").replace("\r\n", "\n") == expected
    manifest = tomllib.loads(expected)
    assert manifest["feature"][0]["default_enabled"] is False
    assert [c["value"] for c in manifest["option"][0]["choice"]] == ["60", "90", "120"]
print("PASS: exact smoothing sources and default-off 60/90/120 catalog in both kit locations")
