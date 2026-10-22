"""Bounded cadence/pixel verification against a running OpenGL test build.

Run during a moving race. Captures stay under ignored analysis/framerate.
This measures source submissions separately from extra rendered frames.
"""
import argparse
import json
import pathlib
import time
from verify_negcon_oracle import query


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", type=int, default=4373)
    parser.add_argument("--output", type=pathlib.Path, default=pathlib.Path("analysis/framerate/runtime-verification.json"))
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    deadline = time.monotonic() + 60
    attempts = []
    while time.monotonic() < deadline:
        state = query(args.port, "gpu_state")
        status = query(args.port, "geometry_history")
        ws = state.get("ws", {})
        renderer = status["renderer"]
        recent = ws.get("cur_frame", 0) - ws.get("last_world3d_frame", 0) <= 4
        if state["depth24"] or not renderer["enabled"] or not renderer["scene"] or not recent:
            time.sleep(0.3)
            continue
        start_frame = query(args.port, "frame")["frame"]
        start = time.perf_counter()
        time.sleep(5)
        end_status = query(args.port, "geometry_history")
        end_frame = query(args.port, "frame")["frame"]
        elapsed = time.perf_counter() - start
        end = end_status["renderer"]
        sample = dict(seconds=elapsed, vblank_hz=(end_frame-start_frame)/elapsed,
                      source_hz=(end_status["geometry"]["projection"]["epoch"]-status["geometry"]["projection"]["epoch"])/elapsed,
                      midpoint_hz=(end["midpoints"]-renderer["midpoints"])/elapsed,
                      original_hz=(end["originals"]-renderer["originals"])/elapsed,
                      start=status, end=end_status)
        attempts.append(sample)
        if not (57 <= sample["vblank_hz"] <= 63 and
                27 <= sample["source_hz"] <= 33 and
                27 <= sample["midpoint_hz"] <= 33 and
                27 <= sample["original_hz"] <= 33):
            continue
        try:
            query(args.port, "geometry_pair", arm=1)
            time.sleep(0.1)
            pair = query(args.port, "geometry_pair", path=str(args.output.with_suffix("").resolve()))
        except RuntimeError:
            continue
        if not pair["changed_pixels"]:
            continue
        result = dict(passed=True, cadence=sample, capture=pair,
                      total_rendered_hz=sample["midpoint_hz"]+sample["original_hz"],
                      simulation_timing_patch=False,
                      latency_scope="Scheduling adds one VBlank; no physical input-to-photon measurement.")
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        print(f"PASS: source {sample['source_hz']:.2f} Hz; geometric display {result['total_rendered_hz']:.2f} fps; {pair['changed_pixels']} changed midpoint pixels")
        return
    args.output.write_text(json.dumps(dict(passed=False, attempts=attempts,
        reason="No complete moving race sample in the bounded window; enter a race and retry."), indent=2), encoding="utf-8")
    raise SystemExit("No complete moving race sample; enter a race and retry.")


if __name__ == "__main__":
    main()
