"""Regression check for viewer refactors: renders the same scenes with two argon builds (--shot) and reports how
many pixels differ, then loads every track with both builds and compares their load summaries (draws, triangles,
textures, route, viability).

The scenes animate with the wall clock (and the drive runs on real time), so a few percent can differ between two
runs of the same build: compare against a baseline-vs-baseline run before reading much into a number.

usage: python tools/viewer/shot_compare.py <baseline argon.exe> <candidate argon.exe> <output folder>
"""
import subprocess
import sys
from pathlib import Path

from PIL import Image, ImageChops

PROJECT = Path(__file__).resolve().parents[2]
COMMON_ARGS = ["--game", str(PROJECT), "--mute", "--size", "960", "540", "--nopanel"]
SCENES = {
    "editor_bracken": ["--editor", "--track", "bracken"],
    "editor_fuusoku_water": ["--editor", "--track", "fuusoku"],
    "editor_silver_top": ["--editor", "--track", "silver", "--cam", "top"],
    "drive_chase": ["--track", "bracken", "--drive-time", "6", "--view", "2"],
    "drive_cockpit": ["--track", "draco", "--drive-time", "6", "--view", "0"],
    "menu_main": ["--menu", "--menu-page", "0"],
    "menu_characters": ["--menu", "--menu-page", "1"],
    "menu_courses": ["--menu", "--menu-page", "4"],
    "loader_flyby": ["--quick-race", "0", "--frames", "30"],
    "race_countdown": ["--quick-race", "0", "--race-go", "--countdown-at", "1.5"],
}
SUMMARY_PREFIXES = ("track ", "route:", "viability:", "warning:")
CHANGED_CHANNEL_THRESHOLD = 24


def run_viewer(exe, args):
    return subprocess.run([str(exe)] + COMMON_ARGS + args, capture_output=True, text=True, timeout=180)


def render_scene(exe, scene_args, shot_path):
    result = run_viewer(exe, scene_args + ["--shot", str(shot_path)])
    if not shot_path.exists():
        raise RuntimeError(f"{exe} wrote no shot for {scene_args}:\n{result.stdout}\n{result.stderr}")


def changed_fraction(baseline_path, candidate_path, diff_path):
    baseline = Image.open(baseline_path).convert("RGB")
    candidate = Image.open(candidate_path).convert("RGB")
    diff = ImageChops.difference(baseline, candidate)
    mask = diff.convert("L").point(lambda level: 255 if level > CHANGED_CHANNEL_THRESHOLD else 0)
    diff.save(diff_path)
    return mask.histogram()[255] / (mask.width * mask.height)


def compare_scenes(baseline_exe, candidate_exe, out_dir):
    for name, scene_args in SCENES.items():
        baseline_shot = out_dir / f"{name}_baseline.png"
        candidate_shot = out_dir / f"{name}_candidate.png"
        render_scene(baseline_exe, scene_args, baseline_shot)
        render_scene(candidate_exe, scene_args, candidate_shot)
        fraction = changed_fraction(baseline_shot, candidate_shot, out_dir / f"{name}_diff.png")
        print(f"{name:22s} {fraction * 100:6.2f} % of pixels differ")


def load_summary(exe, track, out_dir):
    result = run_viewer(exe, ["--editor", "--track", track, "--frames", "1", "--shot", str(out_dir / "summary.png")])
    lines = (result.stdout + result.stderr).splitlines()
    summary = [line for line in lines if line.startswith(SUMMARY_PREFIXES)]
    return [line.split(", loaded")[0].rsplit(",", 1)[0] if line.startswith("track ") else line for line in summary]


def compare_track_loads(baseline_exe, candidate_exe, out_dir):
    tracks = sorted(p.name for p in (PROJECT / "game_data" / "data" / "tracks").iterdir() if (p / "model.cmm").exists())
    for track in tracks:
        same = load_summary(baseline_exe, track, out_dir) == load_summary(candidate_exe, track, out_dir)
        print(f"track {track:14s} {'same load summary' if same else 'LOAD SUMMARY DIFFERS'}")


if __name__ == "__main__":
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    output = Path(sys.argv[3])
    output.mkdir(parents=True, exist_ok=True)
    compare_scenes(Path(sys.argv[1]), Path(sys.argv[2]), output)
    compare_track_loads(Path(sys.argv[1]), Path(sys.argv[2]), output)
