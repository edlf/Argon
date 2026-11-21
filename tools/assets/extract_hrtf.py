"""Extracts the game's HRTF tables from default.xbe (USA) into <game>/audio/hrtf.bin, so the viewer does not read the
XBE at run time (audio\\hrtf.cpp, docs\\audio\\mix-levels.md "Mix levels", docs\\xbe-data\\README.md).

  filters  0x1459d8: 2222 Sensaura filters of 32 bytes (31 tap codes: sign, 3-bit exponent, 4-bit mantissa, then the interaural delay in samples)
  index    0x156f98: 61 x 31 little-endian u16 filter numbers ([azimuth / 3][(elevation + 90) / 6])

hrtf.bin: "ARHR", u32 version 1, u32 filter count, u32 index count, the filters' bytes, then the index's, as in the XBE.

usage: python -I tools/assets/extract_hrtf.py [--game <folder>] [--force]
"""
import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'xbe'))
import xbe as xbe_file  # noqa: E402  (tools\xbe\xbe.py)

FILTERS_VA, FILTER_COUNT, FILTER_BYTES = 0x1459D8, 2222, 32
INDEX_VA, INDEX_COUNT = 0x156F98, 61 * 31
MAGIC, VERSION = b"ARHR", 1


def section_offset(xbe, va, size):
    """The file offset of `size` bytes at virtual address `va`: the first and the last byte in the same section."""
    start, last = xbe_file.va2off(xbe, va), xbe_file.va2off(xbe, va + size - 1)
    if start is None or last != start + size - 1:
        raise ValueError(f"no section holds 0x{va:x} (+{size}): not the USA default.xbe?")
    return start


def extract(xbe):
    filters = section_offset(xbe, FILTERS_VA, FILTER_COUNT * FILTER_BYTES)
    index = section_offset(xbe, INDEX_VA, INDEX_COUNT * 2)
    filter_bytes = xbe[filters:filters + FILTER_COUNT * FILTER_BYTES]
    index_bytes = xbe[index:index + INDEX_COUNT * 2]
    if max(struct.unpack(f"<{INDEX_COUNT}H", index_bytes)) + 1 >= FILTER_COUNT:
        raise ValueError("the HRTF index points past its filters: not the USA default.xbe?")
    return MAGIC + struct.pack("<3I", VERSION, FILTER_COUNT, INDEX_COUNT) + filter_bytes + index_bytes


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--game", type=Path, default=Path(__file__).resolve().parents[2] / "game_data")
    parser.add_argument("--force", action="store_true", help="write hrtf.bin even when it is newer than default.xbe")
    args = parser.parse_args()
    xbe_path, out_path = args.game / "default.xbe", args.game / "audio" / "hrtf.bin"
    if not xbe_path.is_file():
        sys.exit(f"{xbe_path} not found")
    if out_path.is_file() and out_path.stat().st_mtime >= xbe_path.stat().st_mtime and not args.force:
        print(f"{out_path} is up to date")
        return
    try:
        data = extract(xbe_path.read_bytes())
    except ValueError as error:
        sys.exit(f"{xbe_path}: {error}")
    out_path.write_bytes(data)
    print(f"wrote {out_path} ({len(data)} bytes)")


if __name__ == "__main__":
    main()
