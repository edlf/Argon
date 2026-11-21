"""Read the game's save signing key from a running xemu (GDB stub on tcp 1234, tools\\xemu\\run_xemu.ps1) into keys.json.

usage: python -I tools\\saves\\signature_key.py [key file outside the repo] [--xbe FILE] [--check SAVE] [--force]
Without a key file it sets XboxSignatureKey in the repo's keys.json (kept out of git), keeping its other entries.

When the kernel starts an XBE it derives the title's signing key, XboxSignatureKey (kernel export 325) =
HMAC-SHA1(XboxCERTKey, the certificate's signature key)[:16], the key XCalculateSignature signs the saves with
(docs\\saves\\README.md). The game imports that export, so its kernel thunk entry points at the 16 bytes once it runs. This
reads them through that entry (the game must be running, any page) and writes them as 32 hex digits for
profiles.py --signature-key / the editor's "Game key file...". The key is never printed. --check verifies a save
(folder or savegame.sav) with it, e.g. one copied out of the HDD untouched: "valid" confirms the key.
"""
import argparse
import json
import struct
import sys
from pathlib import Path

TOOLS = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(TOOLS / 'saves'), str(TOOLS / 'xbe'), str(TOOLS / 'xemu')]
import profiles as qp
import xbe
from qrdbg import Gdb

XBOX_SIGNATURE_KEY_ORDINAL = 325
KERNEL_SPACE = 0x80000000
XBE_HEADER = 0x10000
REPO = TOOLS.parent


class ProbeError(Exception):
    pass


def check_output_path(path, force):
    resolved = path.resolve()
    if resolved == REPO or REPO in resolved.parents:
        raise ProbeError('%s is inside the repo: keep the key file outside it (or leave it out for keys.json)' % path)
    if path.exists() and not force:
        raise ProbeError('%s exists (--force replaces it)' % path)


def store_in_keys_json(path, key):
    """Sets keys.json's XboxSignatureKey, keeping its other entries."""
    document = json.loads(path.read_text(encoding='utf-8')) if path.is_file() else {}
    document[qp.GAME_KEY_NAME] = key.hex().upper()
    path.write_text(json.dumps(document, indent=2) + '\n', encoding='utf-8')


def store_key(args, key):
    if args.out:
        out = Path(args.out)
        out.write_text(key.hex() + '\n', encoding='ascii')
        return out
    store_in_keys_json(qp.KEYS_JSON, key)
    return qp.KEYS_JSON


def running_title_id(gdb):
    certificate = struct.unpack('<I', gdb.read(XBE_HEADER + 0x118, 4))[0]
    return struct.unpack('<I', gdb.read(certificate + xbe.CERTIFICATE_TITLE_ID, 4))[0]


def read_signature_key(gdb, xbe_data):
    expected_title = xbe.certificate_title_id(xbe_data)
    running_title = running_title_id(gdb)
    if running_title != expected_title:
        raise ProbeError('the running title is %08x, not this XBE\'s %08x' % (running_title, expected_title))
    slot = xbe.kernel_import_slot(xbe_data, XBOX_SIGNATURE_KEY_ORDINAL)
    export = struct.unpack('<I', gdb.read(slot, 4))[0]
    if export < KERNEL_SPACE or export == xbe.KERNEL_ORDINAL_FLAG | XBOX_SIGNATURE_KEY_ORDINAL:
        raise ProbeError('the thunk entry %#x is not resolved yet (is the game running?)' % slot)
    return gdb.read(export, 16)


def read_from_xemu(xbe_data):
    gdb = Gdb()
    try:
        return read_signature_key(gdb, xbe_data)
    finally:
        gdb.cont()


def report_check(save_path, key):
    status = qp.SigningKey(key).check(qp.load_save(save_path))
    print('%s: signature %s' % (qp.save_file_path(save_path), status.value))


def build_parser():
    parser = argparse.ArgumentParser(description='Read the game\'s save signing key (XboxSignatureKey) from xemu.')
    parser.add_argument('out', nargs='?', help='a key file to write (outside the repo); left out: keys.json\'s '
                                               'XboxSignatureKey')
    parser.add_argument('--xbe', metavar='FILE', help='the game\'s default.xbe (default: game_data\\default.xbe)')
    parser.add_argument('--check', metavar='SAVE', help='verify this save folder or savegame.sav with the key')
    parser.add_argument('--force', action='store_true', help='replace an existing key file')
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    try:
        if args.out:
            check_output_path(Path(args.out), args.force)
        xbe_path = Path(args.xbe) if args.xbe else qp.default_xbe_path()
        key = read_from_xemu(xbe_path.read_bytes())
        print('%s: written (the game\'s signing key, 16 bytes)' % store_key(args, key))
        if args.check:
            report_check(args.check, key)
        return 0
    except (OSError, ValueError, ProbeError) as error:
        print('error: %s' % error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
