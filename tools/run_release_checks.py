"""Portable synthetic release regressions; no disc or generated game data."""
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
cc = shlex.split(os.environ.get("CC", "cc"))
cxx = shlex.split(os.environ.get("CXX", "c++"))
include = "-I" + str(root / "psxrecomp/runtime/include")

def run(args):
    subprocess.run(args, cwd=root, check=True)

scratch = root / "build-release-checks"
scratch.mkdir(exist_ok=True)
with tempfile.TemporaryDirectory(prefix="checks-", dir=scratch) as directory:
    work = Path(directory)
    suffix = ".exe" if os.name == "nt" else ""
    for name in ("geometry_schedule", "geometry_packet_tag", "hud_packet",
                 "negcon_controls", "mod_vertex", "ship_packet",
                 "ship_distance", "track_distance", "clear_background"):
        binary = work / (name + suffix)
        run(cc + ["-std=c11", "-Wall", "-Wextra", "-Werror", include,
                  str(root / f"tools/test_{name}.c"), "-o", str(binary)])
        run([str(binary)])
    objects = []
    for name in ("crc32", "psx_sha256"):
        obj = work / (name + ".o")
        run(cc + [include, "-c", str(root / f"psxrecomp/runtime/src/{name}.c"),
                  "-o", str(obj)])
        objects.append(str(obj))
    binary = work / ("catalog" + suffix)
    run(cxx + ["-std=c++17", include,
               "-I" + str(root / "psxrecomp/recompiler/lib/toml11"),
               str(root / "tools/test_render60_catalog.cpp"),
               str(root / "psxrecomp/runtime/src/mod_packages.cpp"),
               *objects, "-o", str(binary)])
    mods = work / "mods"
    shutil.copytree(root / "mods/preloaded/packages/wxl.presentation.render60",
                    mods / "bundled/wxl.presentation.render60")
    run([str(binary), str(mods)])
print("PASS: synthetic rendering, controls and persisted frame-rate catalog checks")
