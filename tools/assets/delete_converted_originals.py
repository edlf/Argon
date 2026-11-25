"""Checks the conversions of tools/assets/convert_assets.py and deletes the originals they replace, once verified.

For every original the converter handles (the .bik videos, media/music/**/*.wav, audio/**/*.wav), it reports one of:

  verified          the converted file is there, not older than the original, and passes the converter's own check
                    right now (FLAC: decoded back, the source's samples bit for bit; Ogg: the viewer's decode gives the
                    .wav's length, play length and loop; .ogv: the soundtrack when the .bik has one, every frame)
  not converted     no converted file next to it
  out of date       the converted file is older than the original (convert_assets.py would convert it again)
  failing           the converted file does not pass the check

By default it only reports (a dry run) and the bytes deleting the verified originals would free. With --delete it
deletes those originals, and only those: everything else is kept and listed. Nothing else is ever touched (the story
scenes' speech in data/movies/speech, the saves and their custom music, the converted files). The viewer plays the
converted files without the originals (audio\\game_sound_files.hpp).

usage: python tools/assets/delete_converted_originals.py [--delete] [--ffmpeg <path to ffmpeg>] [--decoder <path to argon_decode>] [--game <folder>]
"""
import argparse
import subprocess
import sys
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))   # convert_assets.py beside it, also under python -I
import convert_assets  # noqa: E402

VERIFIED, NOT_CONVERTED, OUT_OF_DATE, FAILING = "verified", "not converted", "out of date", "failing"
STATUSES = (VERIFIED, NOT_CONVERTED, OUT_OF_DATE, FAILING)
KIND_TITLES = {convert_assets.VIDEO_KIND: "videos", convert_assets.MUSIC_KIND: "music", convert_assets.EFFECT_KIND: "sound effects"}
VERIFICATION_ERRORS = (RuntimeError, ValueError, OSError, subprocess.SubprocessError)
MEGABYTE = 1024 * 1024


@dataclass(frozen=True)
class ConversionCheck:
    """an original's status, why it is kept (empty when verified) and its size in bytes"""
    conversion: convert_assets.Conversion
    status: str
    reason: str
    original_bytes: int


def check_conversion(verifier, conversion):
    """the status of one original; verifier.verify(conversion) raises when the converted file fails its check"""
    original_bytes = conversion.source.stat().st_size
    if not conversion.target.exists():
        return ConversionCheck(conversion, NOT_CONVERTED, f"no {conversion.target.name}", original_bytes)
    if not convert_assets.is_up_to_date(conversion):
        return ConversionCheck(conversion, OUT_OF_DATE, f"{conversion.target.name} is older than the original", original_bytes)
    try:
        verifier.verify(conversion)
    except VERIFICATION_ERRORS as error:
        return ConversionCheck(conversion, FAILING, str(error) or type(error).__name__, original_bytes)
    return ConversionCheck(conversion, VERIFIED, "", original_bytes)


def verified_checks(checks):
    return [check for check in checks if check.status == VERIFIED]


def kept_checks(checks):
    return [check for check in checks if check.status != VERIFIED]


def bytes_to_free(checks):
    return sum(check.original_bytes for check in verified_checks(checks))


def count_by_kind_and_status(checks):
    """{(kind, status): number of originals}"""
    return Counter((check.conversion.kind, check.status) for check in checks)


def summary_lines(checks, deleted):
    """one line per kind with its count in each status, then the total and the bytes freed (or that would be)"""
    counts = count_by_kind_and_status(checks)
    lines = []
    for kind, title in KIND_TITLES.items():
        per_status = ", ".join(f"{counts[(kind, status)]} {status}" for status in STATUSES)
        lines.append(f"{title:<14} {per_status}")
    verified = len(verified_checks(checks))
    freed = bytes_to_free(checks)
    action = "deleted" if deleted else "would delete"
    lines.append(f"{action} {verified} of {len(checks)} originals, {freed / MEGABYTE:.1f} MB ({freed} bytes); {len(checks) - verified} kept")
    return lines


def delete_originals(checks):
    """deletes the verified checks' originals (the source files only)"""
    for check in verified_checks(checks):
        check.conversion.source.unlink()


def check_all(verifier, conversions):
    checks = []
    for number, conversion in enumerate(conversions, 1):
        print(f"\rchecking {number}/{len(conversions)}", end="", flush=True)
        checks.append(check_conversion(verifier, conversion))
    print()
    return checks


def print_kept(checks):
    for check in kept_checks(checks):
        print(f"kept  {check.conversion.source}  ({check.status}: {check.reason})")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--delete", action="store_true", help="delete the verified originals (default: only report)")
    convert_assets.add_tool_arguments(parser)
    args = parser.parse_args()
    conversions = convert_assets.find_conversions(Path(args.game))
    if not conversions:
        print(f"nothing to check: no .bik or .wav originals of the converter under {args.game}")
        return
    verifier = convert_assets.Converter(*convert_assets.find_tools(args), False)
    checks = check_all(verifier, conversions)
    print_kept(checks)
    if args.delete:
        delete_originals(checks)
    print("\n".join(summary_lines(checks, args.delete)))


if __name__ == "__main__":
    main()
