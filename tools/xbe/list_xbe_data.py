"""Lists the data argon takes from default.xbe (USA) as written into its source, for docs\\xbe-data\\README.md.

Two kinds, found by the addresses the source's comments cite:
  tables     an address in the XBE's file-backed data: .rdata 0x18ecc0..0x1a54b0, .data 0x1a54c0..0x1beb58, or the
             DSOUND library's HRTF tables (0x1459d8, 0x156f98); the run-time state past .data's file bytes is not data
  constants  a code address (a value read off an instruction)
Each is counted only where it is cited on a constexpr / const definition or in the comment block right above one.

--check compares each float constant (one literal, one data address on its line) with the float at that address in
<game>/default.xbe and prints the ones that differ.

usage: python -I tools/xbe/list_xbe_data.py [--markdown | --check] [--game <folder>]   (plain lines by default)
"""
import argparse
import re
import struct
import sys
from operator import attrgetter
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FILE_DATA = [(0x18ECC0, 0x1A54B0, ".rdata"), (0x1A54C0, 0x1BEB58, ".data")]
LIBRARY_DATA = {0x1459D8: "DSOUND", 0x156F98: "DSOUND"}
CODE = (0x12000, 0x108A50)
ADDRESS = re.compile(r"\b0x([0-9a-fA-F]{5,6})\b")
DEFINITION = re.compile(r"^\s*(static\s+)?(inline\s+)?(constexpr|const)\s")
FLOAT_CONSTANT = re.compile(r"\bfloat\s+\w+\s*=\s*(-?\d+(?:\.\d*)?(?:e-?\d+)?)f\s*;")


@dataclass
class Use:
    address: int
    section: str
    path: str
    line: int
    text: str


def section_of(address):
    for start, end, name in FILE_DATA:
        if start <= address < end:
            return name
    return LIBRARY_DATA.get(address)


def is_definition(line):
    return bool(DEFINITION.match(line))


def is_comment(line):
    return line.lstrip().startswith("//")


def definition_below(lines, index):
    """The definition a comment block ending at lines[index] describes: the first line after the block, if it is one."""
    below = index + 1
    while below < len(lines) and is_comment(lines[below]):
        below += 1
    return below if below < len(lines) and is_definition(lines[below]) else None


def scan_file(path):
    tables, constants = [], []
    relative = path.relative_to(ROOT).as_posix()
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    for index, line in enumerate(lines):
        defined = index if is_definition(line) else definition_below(lines, index) if is_comment(line) else None
        if defined is None:
            continue
        for match in ADDRESS.finditer(line):
            address = int(match.group(1), 16)
            section = section_of(address)
            use = Use(address, section or ".text", relative, defined + 1, lines[defined].strip())
            if section:
                tables.append(use)
            elif CODE[0] <= address < CODE[1]:
                constants.append(use)
    return tables, constants


def scan():
    tables, constants = [], []
    for path in sorted((ROOT / "src").rglob("*.[ch]pp")):
        found_tables, found_constants = scan_file(path)
        tables += found_tables
        constants += found_constants
    return tables, constants


def unique_by_line(uses):
    seen, kept = set(), []
    for use in uses:
        if (use.path, use.line) not in seen:
            seen.add((use.path, use.line))
            kept.append(use)
    return kept


def markdown_cell(text):
    return text.replace("|", "\\|")


def print_markdown(tables, constants):
    print("| address | section | where | source line |")
    print("|---|---|---|---|")
    for use in sorted(unique_by_line(tables), key=attrgetter("address", "path", "line")):
        print(f"| 0x{use.address:x} | {use.section} | {use.path}:{use.line} | `{markdown_cell(use.text)[:150]}` |")
    print()
    print("| code address | where | definition |")
    print("|---|---|---|")
    for use in sorted(unique_by_line(constants), key=attrgetter("path", "line")):
        print(f"| 0x{use.address:x} | {use.path}:{use.line} | `{markdown_cell(use.text)[:150]}` |")


def print_plain(tables, constants):
    for use in unique_by_line(tables) + unique_by_line(constants):
        print(f"0x{use.address:06x} {use.section:7} {use.path}:{use.line}  {use.text[:120]}")


def xbe_float(xbe, address):
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import xbe as xbe_file
    offset = xbe_file.va2off(xbe, address)
    return None if offset is None else struct.unpack_from("<f", xbe, offset)[0]


def check(tables, xbe):
    """The float constants whose value differs from the XBE's float at their address; returns how many were compared."""
    compared = 0
    for use in unique_by_line(tables):
        literal = FLOAT_CONSTANT.search(use.text)
        if not literal or len(ADDRESS.findall(use.text)) != 1 or use.section not in (".rdata", ".data"):
            continue
        stored = xbe_float(xbe, use.address)
        compared += 1
        if stored is None or abs(stored - float(literal.group(1))) > 1e-6 * max(1.0, abs(stored)):
            print(f"0x{use.address:x} holds {stored}, {use.path}:{use.line} has {literal.group(1)}")
    return compared


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--markdown", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--game", type=Path, default=ROOT / "game_data")
    args = parser.parse_args()
    tables, constants = scan()
    if args.check:
        print(f"{check(tables, (args.game / 'default.xbe').read_bytes())} float constants compared")
    elif args.markdown:
        print_markdown(tables, constants)
    else:
        print_plain(tables, constants)


if __name__ == "__main__":
    main()
