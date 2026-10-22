"""Verify optional store metadata does not change generated guest instructions.

Requires the owner's local executable and a pre-change generated baseline.
All generated fixtures and game-derived proof remain under ignored analysis/.
"""
import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import tomllib


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--emitter", type=pathlib.Path, default=pathlib.Path("build-recompiler/psxrecomp-game.exe"))
    parser.add_argument("--baseline", type=pathlib.Path, default=pathlib.Path("analysis/framerate/baseline-generated"))
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    config_text = (root / "game.toml").read_text(encoding="utf-8")
    config = tomllib.loads(config_text)
    hooks = config["recompiler"]["mod_store_hooks"]
    image = (root / config["game"]["exe"]).read_bytes()
    load = int(config["game"]["load_address"], 0)
    for pc, expected in hooks.items():
        offset = 0x800 + int(pc, 0) - load
        assert int.from_bytes(image[offset:offset + 4], "little") == int(expected, 0), pc
    call = re.compile(r" psx_mod_function_entry\(cpu, 0x[0-9A-Fa-f]{8}u\); /\* presentation metadata after verified store \*/")
    shards, calls = [], 0
    baseline = (root / args.baseline).resolve()
    for old in sorted(baseline.glob("*_full_*.c")):
        new = (root / "generated" / old.name).read_text(encoding="utf-8")
        stripped, n = call.subn("", new)
        assert stripped == old.read_text(encoding="utf-8"), old.name
        shards.append(old.name)
        calls += n
    assert len(shards) == 40 and calls == len(hooks), (len(shards), calls, len(hooks))
    for old in baseline.glob("*_dispatch.c"):
        assert old.read_bytes() == (root / "generated" / old.name).read_bytes()
    work = root / "analysis/framerate/generation-test"
    work.mkdir(parents=True, exist_ok=True)
    text = config_text
    for key, value in [("exe", root / config["game"]["exe"]),
                       ("seeds", root / config["recompiler"]["seeds"]),
                       ("out_dir", work / "output")]:
        text = re.sub(rf'^{key} = "[^"]*"', f'{key} = "{value.as_posix()}"', text, flags=re.M)
    emitter = (root / args.emitter).resolve()
    env = os.environ.copy()
    if os.name == "nt":
        env["PATH"] = "C:/msys64/mingw64/bin;" + env.get("PATH", "")
    pc, expected = next(iter(hooks.items()))
    broken = text.replace(f'"{pc}" = "{expected}"', f'"{pc}" = "0x{int(expected, 0) ^ 1:08X}"')
    fixture = work / "mismatch.toml"
    fixture.write_text(broken, encoding="utf-8")
    result = subprocess.run([str(emitter), "--config", str(fixture)], cwd=root, env=env, capture_output=True)
    assert result.returncode != 0 and b"mod_store_hooks instruction mismatch" in result.stdout + result.stderr
    no_hooks = re.sub(r"\n\[recompiler\.mod_store_hooks\][\s\S]*$", "\n", text)
    fixture = work / "no-hooks.toml"
    fixture.write_text(no_hooks, encoding="utf-8")
    result = subprocess.run([str(emitter), "--config", str(fixture)], cwd=root, env=env, capture_output=True)
    assert result.returncode == 0, (result.stdout + result.stderr)[-2000:]
    for name in shards:
        assert (work / "output" / name).read_bytes() == (baseline / name).read_bytes(), name
    proof = dict(hooks=len(hooks), instruction_words_match_legal_executable=True,
                 game_shards_identical_after_removing_only_metadata=shards,
                 dispatcher_identical=True, bad_instruction_rejected=True,
                 empty_hook_config_identical=True,
                 emitter_sha256=hashlib.sha256(emitter.read_bytes()).hexdigest())
    (root / "analysis/framerate/regeneration-proof.json").write_text(json.dumps(proof, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {calls} verified stores, 40 unchanged guest shards, unchanged dispatch, fail-closed mismatch, empty-config identity")


if __name__ == "__main__":
    main()
