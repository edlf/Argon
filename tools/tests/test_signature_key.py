"""Tests for tools\\saves\\signature_key.py and its XBE helpers (tools\\xbe\\xbe.py): no xemu, a fake XBE and a fake GDB link.

run: python -m unittest discover -s tools\\tests   (test.bat runs it through CTest)
"""
import json
import struct
import sys
import tempfile
import unittest
from pathlib import Path

sys.path[:0] = [str(Path(__file__).resolve().parents[1] / folder) for folder in ('saves', 'xbe', 'xemu')]
import signature_key as probe
import xbe

BASE = 0x10000
CERTIFICATE = 0x178
THUNK_TABLE_OFFSET = 0x200
TITLE_ID = 0x4D530015
SIGNATURE_KEY_ORDINAL = 325
IMPORTS = (1, 2, SIGNATURE_KEY_ORDINAL, 3)
KEY_ADDRESS = 0x80012340
FAKE_KEY = bytes(range(0x10, 0x20))


def fake_xbe():
    """Headers, a certificate and the kernel thunk table, all in one raw section mapped at BASE."""
    data = bytearray(0x300)
    struct.pack_into('<I', data, 0x104, BASE)
    struct.pack_into('<I', data, 0x118, BASE + CERTIFICATE)
    struct.pack_into('<II', data, 0x11C, 1, BASE + 0x2A0)
    struct.pack_into('<IIIII', data, 0x2A0, 0, BASE, len(data), 0, len(data))
    struct.pack_into('<I', data, 0x158, (BASE + THUNK_TABLE_OFFSET) ^ xbe.RETAIL_THUNK_KEY)
    struct.pack_into('<I', data, CERTIFICATE + xbe.CERTIFICATE_TITLE_ID, TITLE_ID)
    for index, ordinal in enumerate(IMPORTS):
        struct.pack_into('<I', data, THUNK_TABLE_OFFSET + 4 * index, xbe.KERNEL_ORDINAL_FLAG | ordinal)
    return bytes(data)


def key_slot():
    return BASE + THUNK_TABLE_OFFSET + 4 * IMPORTS.index(SIGNATURE_KEY_ORDINAL)


class FakeGdb:
    """Guest memory as {address: bytes}: the running XBE's header and certificate, the resolved thunk, the key."""

    def __init__(self, title_id=TITLE_ID, thunk_value=KEY_ADDRESS):
        self.memory = {
            BASE + 0x118: struct.pack('<I', BASE + CERTIFICATE),
            BASE + CERTIFICATE + xbe.CERTIFICATE_TITLE_ID: struct.pack('<I', title_id),
            key_slot(): struct.pack('<I', thunk_value),
            KEY_ADDRESS: FAKE_KEY,
        }

    def read(self, address, count):
        return self.memory[address][:count]


class XbeKernelImportTests(unittest.TestCase):
    def test_thunk_table_is_decoded_with_the_retail_key(self):
        self.assertEqual(xbe.kernel_thunk_table(fake_xbe()), BASE + THUNK_TABLE_OFFSET)

    def test_import_slot_is_the_entry_of_that_ordinal(self):
        self.assertEqual(xbe.kernel_import_slot(fake_xbe(), SIGNATURE_KEY_ORDINAL), key_slot())

    def test_missing_import_is_an_error(self):
        with self.assertRaises(ValueError):
            xbe.kernel_import_slot(fake_xbe(), 99)

    def test_title_id_is_read_from_the_certificate(self):
        self.assertEqual(xbe.certificate_title_id(fake_xbe()), TITLE_ID)


class ReadSignatureKeyTests(unittest.TestCase):
    def test_key_is_read_through_the_resolved_thunk(self):
        key = probe.read_signature_key(FakeGdb(), fake_xbe())

        self.assertEqual(key, FAKE_KEY)

    def test_another_running_title_is_refused(self):
        with self.assertRaises(probe.ProbeError):
            probe.read_signature_key(FakeGdb(title_id=0x12345678), fake_xbe())

    def test_unresolved_thunk_is_refused(self):
        unresolved = xbe.KERNEL_ORDINAL_FLAG | SIGNATURE_KEY_ORDINAL

        with self.assertRaises(probe.ProbeError):
            probe.read_signature_key(FakeGdb(thunk_value=unresolved), fake_xbe())


class OutputPathTests(unittest.TestCase):
    def test_key_file_inside_the_repo_is_refused(self):
        with self.assertRaises(probe.ProbeError):
            probe.check_output_path(probe.REPO / 'tools' / 'game.key', force=False)

    def test_existing_key_file_needs_force(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'game.key'
            path.write_text('x', encoding='ascii')

            with self.assertRaises(probe.ProbeError):
                probe.check_output_path(path, force=False)

    def test_existing_key_file_is_replaced_with_force(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'game.key'
            path.write_text('x', encoding='ascii')

            probe.check_output_path(path, force=True)


class KeysJsonStoreTests(unittest.TestCase):
    def test_key_is_stored_under_xbox_signature_key(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'keys.json'

            probe.store_in_keys_json(path, FAKE_KEY)

            document = json.loads(path.read_text(encoding='utf-8'))
        self.assertEqual(bytes.fromhex(document['XboxSignatureKey']), FAKE_KEY)

    def test_other_entries_are_kept(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'keys.json'
            path.write_text('{"XbeSignature": "00", "XboxCERTKey": ""}', encoding='utf-8')

            probe.store_in_keys_json(path, FAKE_KEY)

            document = json.loads(path.read_text(encoding='utf-8'))
        self.assertEqual((document['XbeSignature'], document['XboxCERTKey']), ('00', ''))


if __name__ == '__main__':
    unittest.main()
