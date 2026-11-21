"""tools\\saves\\profiles.py: the savegame.sav layout, the save folder, the signature scheme and the viewer JSON conversion.
The signing tests use a made-up key (bytes(range(16))) and a made-up XBE header: no real key is involved."""
import hashlib
import hmac
import json
import random
import struct
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'saves'))

import profiles as qp  # noqa: E402

FIXTURES = Path(__file__).resolve().parent / 'fixtures'
FAKE_CERT_KEY = bytes(range(16))
FAKE_CERTIFICATE_KEY = bytes(range(0xA0, 0xB0))
FAKE_BASE = 0x10000
FAKE_CERTIFICATE = 0x178


def fake_xbe():
    data = bytearray(0x300)
    struct.pack_into('<I', data, 0x104, FAKE_BASE)
    struct.pack_into('<I', data, 0x118, FAKE_BASE + FAKE_CERTIFICATE)
    data[FAKE_CERTIFICATE + 0xC0:FAKE_CERTIFICATE + 0xD0] = FAKE_CERTIFICATE_KEY
    return bytes(data)


def fake_signing_key():
    return qp.derive_signing_key(FAKE_CERT_KEY, FAKE_CERTIFICATE_KEY)


def synthetic_save_bytes():
    """Random content behind a NUL-terminated name, as a played save with stack junk would look."""
    noise = random.Random(1234)
    data = bytearray(noise.getrandbits(8) for _ in range(qp.SAVE_BYTES))
    name = 'PLAYER1'.encode('utf-16-le') + b'\0\0'
    data[:len(name)] = name
    return bytes(data)


def fixture_document():
    return json.loads((FIXTURES / 'viewer_PLAYER1.json').read_text(encoding='utf-8'))


def viewer_json_value(text, key):
    """A copy of app_profile.cpp's jsonValue: the first "key", then a string or a bare token."""
    position = text.find('"%s"' % key)
    if position < 0:
        return ''
    position = text.find(':', position) + 1
    while text[position].isspace():
        position += 1
    if text[position] == '"':
        value = ''
        position += 1
        while text[position] != '"':
            if text[position] == '\\':
                position += 1
                value += '\n' if text[position] == 'n' else text[position]
            else:
                value += text[position]
            position += 1
        return value
    end = position
    while text[end] not in ',}' and not text[end].isspace():
        end += 1
    return text[position:end]


def converted(document_changes):
    document = {'name': 'PLAYER1'}
    document.update(document_changes)
    return qp.from_viewer_json(document)


def played_save():
    """A save the viewer JSON can hold exactly (the padding zero, flags 0 / 1, the speed classes equal)."""
    save = qp.new_save('PLAYER1')
    profile = save.profile
    profile.best_lap_bits[7] = qp.float32_bits(63.123456)
    profile.camera = 3
    profile.characters = 0x10ff
    profile.set_open_class(3)
    profile.records[4].cash = 72564
    profile.records[4].upgrades = [4, 2, 2]
    profile.records[4].trophies = [1, 1, 0, 0, 0]
    profile.records[4].tourney_progress = [-1, -1, 2, 0, -1]
    profile.records[4].hi_tourney[6] = 270206
    profile.records[5].hi_quick[17] = 227182
    profile.game_speed = 2
    profile.tourney_level = 2
    profile.infinite_turbo = 1
    profile.cheats_on[qp.INFINITE_TURBO_CHEAT] = 1
    profile.unused_c84 = 9
    profile.tracks = 0x107f
    profile.mars = 1
    profile.eggs[2] = 1
    profile.eggs_on[2] = 1
    profile.cheats[6] = 1
    profile.music_default = 0
    profile.hints = [3, 30, 39]
    profile.redshift_wins = [1, 0, 1]
    save.settings.sound[5] = 7
    save.playlist[0] = qp.PlaylistSlot(4, 1, 'Song'.encode('utf-16-le').ljust(0x40, b'\0'))
    return save


class SaveLayoutTests(unittest.TestCase):
    def test_unedited_save_round_trips_byte_identical(self):
        data = synthetic_save_bytes()

        save = qp.SaveGame.parse(data)

        self.assertEqual(save.to_bytes(), data)

    def test_new_save_is_5865_bytes(self):
        save = qp.new_save('PLAYER1')

        data = save.to_bytes()

        self.assertEqual(len(data), 0x16E9)

    def test_name_is_read_up_to_its_nul(self):
        data = synthetic_save_bytes()

        save = qp.SaveGame.parse(data)

        self.assertEqual(save.name, 'PLAYER1')

    def test_rename_zeroes_the_bytes_after_the_name(self):
        save = qp.SaveGame.parse(synthetic_save_bytes())

        save.rename('ME')

        self.assertEqual(save.to_bytes()[:0x100], 'ME'.encode('utf-16-le') + bytes(0xFC))

    def test_parse_rejects_a_file_of_the_wrong_size(self):
        data = bytes(qp.SAVE_BYTES + 1)

        with self.assertRaises(ValueError):
            qp.SaveGame.parse(data)

    def test_rename_rejects_more_than_nine_characters(self):
        save = qp.new_save('PLAYER1')

        with self.assertRaises(ValueError):
            save.rename('TENLETTERS')

    def test_profile_block_sits_at_0x100_with_the_cash_at_its_record(self):
        save = qp.new_save('PLAYER1')
        save.profile.records[1].cash = 0x12345678

        data = save.to_bytes()

        self.assertEqual(struct.unpack_from('<i', data, 0x100 + 0x174 + 0xB0)[0], 0x12345678)

    def test_tracks_sit_at_profile_0xc88(self):
        save = qp.new_save('PLAYER1')
        save.profile.tracks = 0xA5A5A5A5

        data = save.to_bytes()

        self.assertEqual(struct.unpack_from('<I', data, 0x100 + 0xC88)[0], 0xA5A5A5A5)

    def test_sound_settings_come_before_game_settings(self):
        save = qp.new_save('PLAYER1')

        data = save.to_bytes()

        self.assertEqual(data[0xDC4:0xDD5], bytes([0, 0, 0, 4, 4, 4, 0, 0, 0, 1, 1, 1, 1, 0, 4, 2, 0]))


class NewProfileTests(unittest.TestCase):
    def test_new_profile_has_the_reset_values(self):
        profile = qp.new_save('PLAYER1').profile

        values = (profile.best_lap_bits[0], profile.camera, profile.characters, profile.speed_class, profile.tracks,
                  profile.mars, profile.tourney_class, profile.music_default, profile.records[0].tourney_progress)

        self.assertEqual(values, (0x497423F0, 1, 0x7F, 1, 0x7F, 0, 1, 1, [-1] * 5))

    def test_new_profile_has_32_empty_playlist_slots(self):
        playlist = qp.new_save('PLAYER1').playlist

        self.assertTrue(len(playlist) == 32 and all(slot.pack() == b'\xff' * 8 + bytes(0x40) for slot in playlist))

    def test_cheat_name_opens_the_cheat_menu(self):
        profile = qp.new_save('CHEATER').profile

        self.assertEqual(profile.cheat_menu, 1)

    def test_new_profile_is_zero_percent(self):
        profile = qp.new_save('PLAYER1').profile

        self.assertEqual(profile.percent_complete(), 0)


class PercentCompleteTests(unittest.TestCase):
    def test_percent_counts_trophy_levels_later_characters_eggs_and_redshift_wins(self):
        profile = qp.new_save('PLAYER1').profile
        profile.records[0].trophies = [1, 0, 1, 0, 0]   # level 3
        profile.characters = 0x7F | 1 << 8 | 1 << 15  # two later characters
        profile.eggs[1] = 1
        profile.redshift_wins = [1, 1, 1]               # 1 + 1 + 2

        percent = profile.percent_complete()

        self.assertEqual(percent, 3 + 2 + 1 + 4)


def all_won_profile():
    profile = qp.new_save('PLAYER1').profile
    profile.win_all_tournaments()
    return profile


class WinAllTournamentsTests(unittest.TestCase):
    def test_every_character_holds_every_trophy(self):
        self.assertEqual({tuple(record.trophies) for record in all_won_profile().records}, {(1, 1, 1, 1, 1)})

    def test_no_tournament_is_left_running(self):
        self.assertEqual({tuple(record.tourney_progress) for record in all_won_profile().records}, {(-1,) * 5})

    def test_every_character_and_course_route_is_open(self):
        profile = all_won_profile()

        self.assertEqual((profile.characters, profile.tracks, profile.mars), (0xFFFF, 0xFFFF, 1))

    def test_redshift_class_opens_with_its_tournament_level(self):
        profile = all_won_profile()

        self.assertEqual((profile.speed_class, profile.tourney_class), (4, 4))

    def test_every_easter_egg_is_unlocked(self):
        self.assertEqual(all_won_profile().eggs, [1] * 8)

    def test_percent_is_all_but_the_redshift_quick_race_wins(self):
        self.assertEqual(all_won_profile().percent_complete(), 96)

    def test_cash_and_upgrades_are_kept(self):
        profile = qp.new_save('PLAYER1').profile
        profile.records[3].cash, profile.records[3].upgrades = 12345, [1, 2, 3]

        profile.win_all_tournaments()

        self.assertEqual((profile.records[3].cash, profile.records[3].upgrades), (12345, [1, 2, 3]))


class SaveFolderTests(unittest.TestCase):
    def test_folder_id_of_player1(self):
        self.assertEqual(qp.save_folder_id('PLAYER1'), '11C90F515484')

    def test_folder_id_of_the_lap_records_save(self):
        self.assertEqual(qp.save_folder_id('Fastest Lap Data'), '9618834AAC57')

    def test_folder_id_of_the_free_space_probe(self):
        self.assertEqual(qp.save_folder_id('_SAVE'), '0041163B1366')

    def test_profile_save_name_is_the_infotext_prefix_and_the_name(self):
        self.assertEqual(qp.profile_save_name('ET'), 'Profile: ET')

    def test_profile_folder_of_a_real_xbox_save(self):
        self.assertEqual(qp.profile_folder_id('PLAYER1'), 'B1CD4EB5DDBF')

    def test_profile_folder_of_another_real_xbox_save(self):
        self.assertEqual(qp.profile_folder_id('ET'), 'FF5B38DCE470')

    def test_save_meta_is_byte_for_byte_a_real_xbox_one(self):
        real = b'\xff\xfe' + 'Name=Profile: ET\r\n'.encode('utf-16-le')

        self.assertEqual(qp.save_meta_bytes(qp.profile_save_name('ET')), real)

    def test_written_folder_has_the_profile_save_name_in_save_meta(self):
        with tempfile.TemporaryDirectory() as folder:
            qp.write_save_folder(qp.new_save('ET'), folder)

            name = qp.save_meta_name((Path(folder) / 'SaveMeta.xbx').read_bytes())
        self.assertEqual(name, 'Profile: ET')

    def test_save_meta_is_utf16_with_bom_and_crlf(self):
        data = qp.save_meta_bytes('PLAYER1')

        self.assertEqual(data, b'\xff\xfe' + 'Name=PLAYER1\r\n'.encode('utf-16-le'))

    def test_save_meta_name_reads_back(self):
        data = qp.save_meta_bytes('PLAYER1')

        self.assertEqual(qp.save_meta_name(data), 'PLAYER1')

    def test_writing_into_another_profiles_folder_is_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            qp.write_save_folder(qp.new_save('OTHER'), folder)

            with self.assertRaises(ValueError):
                qp.write_save_folder(qp.new_save('PLAYER1'), folder)

    def test_written_folder_loads_back(self):
        save = played_save()
        with tempfile.TemporaryDirectory() as folder:
            qp.write_save_folder(save, folder)

            loaded = qp.load_save(folder)

        self.assertEqual(loaded.to_bytes(), save.to_bytes())

    def test_renamed_profile_overwrites_save_meta_with_its_new_name(self):
        with tempfile.TemporaryDirectory() as folder:
            qp.write_save_folder(qp.new_save('OLDNAME'), folder)
            renamed = qp.new_save('OLDNAME')
            renamed.rename('NEWNAME')

            qp.write_save_folder(renamed, folder, previous_name='OLDNAME')

            name = qp.save_meta_name((Path(folder) / 'SaveMeta.xbx').read_bytes())
        self.assertEqual(name, 'Profile: NEWNAME')

    def test_folder_named_by_another_hash_is_reported(self):
        with tempfile.TemporaryDirectory() as parent:
            folder = Path(parent) / qp.profile_folder_id('OTHER')

            problems = qp.write_save_folder(qp.new_save('PLAYER1'), folder)

        self.assertEqual(len(problems), 1)

    def test_folder_named_by_its_hash_reports_nothing(self):
        with tempfile.TemporaryDirectory() as parent:
            folder = Path(parent) / qp.profile_folder_id('PLAYER1')

            problems = qp.write_save_folder(qp.new_save('PLAYER1'), folder)

        self.assertEqual(problems, [])


def save_folder_with_stale_meta(parent):
    """PLAYER1's folder whose SaveMeta.xbx names another profile."""
    folder = Path(parent) / qp.profile_folder_id('PLAYER1')
    qp.write_save_folder(qp.new_save('PLAYER1'), folder)
    (folder / 'SaveMeta.xbx').write_bytes(qp.save_meta_bytes('STALE'))
    return folder


class RewriteSaveTests(unittest.TestCase):
    def test_stale_save_meta_is_rewritten_with_the_profile_name(self):
        with tempfile.TemporaryDirectory() as parent:
            folder = save_folder_with_stale_meta(parent)

            qp.rewrite_save(folder, qp.load_save(folder))

            name = qp.save_meta_name((folder / 'SaveMeta.xbx').read_bytes())
        self.assertEqual(name, 'Profile: PLAYER1')

    def test_missing_save_meta_is_written_for_a_save_folder(self):
        with tempfile.TemporaryDirectory() as parent:
            folder = save_folder_with_stale_meta(parent)
            (folder / 'SaveMeta.xbx').unlink()

            qp.rewrite_save(folder, qp.load_save(folder))

            written = (folder / 'SaveMeta.xbx').is_file()
        self.assertTrue(written)

    def test_lone_savegame_file_gets_no_save_meta(self):
        with tempfile.TemporaryDirectory() as parent:
            file = Path(parent) / 'savegame.sav'
            file.write_bytes(qp.new_save('PLAYER1').to_bytes())

            qp.rewrite_save(file, qp.load_save(file))

            written = (Path(parent) / 'SaveMeta.xbx').exists()
        self.assertFalse(written)


class RemoveSaveFolderTests(unittest.TestCase):
    def test_save_folder_is_removed(self):
        with tempfile.TemporaryDirectory() as parent:
            folder = Path(parent) / qp.profile_folder_id('PLAYER1')
            qp.write_save_folder(qp.new_save('PLAYER1'), folder)

            qp.remove_save_folder(folder)

            exists = folder.exists()
        self.assertFalse(exists)

    def test_other_files_in_the_folder_are_kept(self):
        with tempfile.TemporaryDirectory() as parent:
            folder = Path(parent) / qp.profile_folder_id('PLAYER1')
            qp.write_save_folder(qp.new_save('PLAYER1'), folder)
            (folder / 'notes.txt').write_text('mine', encoding='ascii')

            qp.remove_save_folder(folder)

            left = sorted(path.name for path in folder.iterdir())
        self.assertEqual(left, ['notes.txt'])


class SignatureTests(unittest.TestCase):
    def test_signing_key_is_hmac_of_the_certificate_key_cut_to_16_bytes(self):
        expected = hmac.new(FAKE_CERT_KEY, FAKE_CERTIFICATE_KEY, hashlib.sha1).digest()[:16]
        body = b'body'

        signature = fake_signing_key().signature(body)

        self.assertEqual(signature, hmac.new(expected, body, hashlib.sha1).digest())

    def test_signature_covers_everything_before_the_last_20_bytes(self):
        save = qp.new_save('PLAYER1')
        key = fake_signing_key()

        save.sign(key)

        data = save.to_bytes()
        self.assertEqual(data[-20:], key.signature(data[:-20]))

    def test_signed_save_checks_valid(self):
        save = qp.new_save('PLAYER1')
        save.sign(fake_signing_key())

        self.assertIs(fake_signing_key().check(save), qp.SignatureStatus.VALID)

    def test_edited_save_checks_invalid(self):
        save = qp.new_save('PLAYER1')
        save.sign(fake_signing_key())

        save.profile.records[0].cash = 1

        self.assertIs(fake_signing_key().check(save), qp.SignatureStatus.INVALID)

    def test_no_key_reports_not_checked(self):
        save = qp.new_save('PLAYER1')

        self.assertIs(qp.MissingSigningKey().check(save), qp.SignatureStatus.NOT_CHECKED)

    def test_no_key_cannot_sign(self):
        save = qp.new_save('PLAYER1')

        with self.assertRaises(qp.SigningKeyMissing):
            save.sign(qp.MissingSigningKey())

    def test_key_file_allows_whitespace(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'key.txt'
            path.write_text('0001 0203 0405 0607\n08090a0b0c0d0e0f\n', encoding='ascii')

            key = qp.read_key_file(path)

        self.assertEqual(key, FAKE_CERT_KEY)

    def test_bad_key_file_error_does_not_show_its_content(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'key.txt'
            path.write_text('00112233SECRET', encoding='ascii')

            with self.assertRaises(ValueError) as raised:
                qp.read_key_file(path)

        self.assertNotIn('SECRET', str(raised.exception))


def write_key_file(folder, name, key):
    path = Path(folder) / name
    path.write_text(key.hex(), encoding='ascii')
    return str(path)


def write_fake_xbe(folder):
    path = Path(folder) / 'default.xbe'
    path.write_bytes(fake_xbe())
    return str(path)


class KeyFilesTests(unittest.TestCase):
    def test_game_key_signs_as_the_key_derived_from_the_cert_key(self):
        derived = hmac.new(FAKE_CERT_KEY, FAKE_CERTIFICATE_KEY, hashlib.sha1).digest()[:16]
        with tempfile.TemporaryDirectory() as folder:
            files = qp.KeyFiles(signature_key=write_key_file(folder, 'game.key', derived), keys_json=None)

            key = qp.load_signing_key(files)

        self.assertEqual(key.signature(b'body'), fake_signing_key().signature(b'body'))

    def test_cert_key_with_the_xbe_derives_the_game_key(self):
        with tempfile.TemporaryDirectory() as folder:
            files = qp.KeyFiles(cert_key=write_key_file(folder, 'cert.key', FAKE_CERT_KEY), xbe=write_fake_xbe(folder),
                                keys_json=None)

            key = qp.load_signing_key(files)

        self.assertEqual(key.signature(b'body'), fake_signing_key().signature(b'body'))

    def test_game_key_is_used_when_both_are_given(self):
        game_key = bytes(16)
        with tempfile.TemporaryDirectory() as folder:
            files = qp.KeyFiles(write_key_file(folder, 'game.key', game_key), write_key_file(folder, 'cert.key', FAKE_CERT_KEY),
                                write_fake_xbe(folder), keys_json=None)

            key = qp.load_signing_key(files)

        self.assertEqual(key.signature(b'body'), hmac.new(game_key, b'body', hashlib.sha1).digest())

    def test_no_key_files_give_no_key(self):
        self.assertFalse(qp.load_signing_key(qp.KeyFiles(keys_json=None)).available)


def write_keys_json(folder, entries):
    path = Path(folder) / 'keys.json'
    path.write_text(json.dumps({name: key.hex() for name, key in entries.items()}), encoding='utf-8')
    return str(path)


class KeysJsonTests(unittest.TestCase):
    def test_game_key_from_keys_json_is_used_as_it_is(self):
        game_key = bytes(16)
        with tempfile.TemporaryDirectory() as folder:
            files = qp.KeyFiles(keys_json=write_keys_json(folder, {'XboxSignatureKey': game_key}))

            key = qp.load_signing_key(files)

        self.assertEqual(key.signature(b'body'), hmac.new(game_key, b'body', hashlib.sha1).digest())

    def test_cert_key_from_keys_json_goes_with_its_xbe_signature(self):
        entries = {'XboxCERTKey': FAKE_CERT_KEY, 'XbeSignature': FAKE_CERTIFICATE_KEY}
        with tempfile.TemporaryDirectory() as folder:
            files = qp.KeyFiles(keys_json=write_keys_json(folder, entries))

            key = qp.load_signing_key(files)

        self.assertEqual(key.signature(b'body'), fake_signing_key().signature(b'body'))

    def test_empty_entries_give_no_key(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'keys.json'
            path.write_text('{"XbeSignature": "", "XboxCERTKey": ""}', encoding='utf-8')

            key = qp.load_signing_key(qp.KeyFiles(keys_json=str(path)))

        self.assertFalse(key.available)

    def test_key_file_wins_over_keys_json(self):
        derived = fake_signing_key().signature(b'body')
        with tempfile.TemporaryDirectory() as folder:
            files = qp.KeyFiles(cert_key=write_key_file(folder, 'cert.key', FAKE_CERT_KEY), xbe=write_fake_xbe(folder),
                                keys_json=write_keys_json(folder, {'XboxSignatureKey': bytes(16)}))

            key = qp.load_signing_key(files)

        self.assertEqual(key.signature(b'body'), derived)

    def test_source_names_the_keys_json_entry_without_the_key(self):
        game_key = bytes(range(0x30, 0x40))
        with tempfile.TemporaryDirectory() as folder:
            path = write_keys_json(folder, {'XboxSignatureKey': game_key})

            source, _ = qp.chosen_signing_key(qp.KeyFiles(keys_json=path))

        self.assertEqual((source, game_key.hex() in source.lower()), ('%s (XboxSignatureKey)' % path, False))

    def test_keys_json_given_as_a_key_file_points_at_keys(self):
        with tempfile.TemporaryDirectory() as folder:
            path = write_keys_json(folder, {'XboxSignatureKey': bytes(16)})

            with self.assertRaises(ValueError) as raised:
                qp.load_signing_key(qp.KeyFiles(signature_key=path, keys_json=None))

        self.assertIn('--keys', str(raised.exception))

    def test_source_without_any_key_says_so(self):
        source, _ = qp.chosen_signing_key(qp.KeyFiles(keys_json=None))

        self.assertEqual(source, 'no key')

    def test_bad_entry_error_does_not_show_it(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'keys.json'
            path.write_text('{"XboxCERTKey": "00112233SECRET"}', encoding='utf-8')

            with self.assertRaises(ValueError) as raised:
                qp.load_signing_key(qp.KeyFiles(keys_json=str(path)))

        self.assertNotIn('SECRET', str(raised.exception))


class ViewerToOriginalTests(unittest.TestCase):
    def test_characters(self):
        self.assertEqual(converted({'characters': '0x107f'}).profile.characters, 0x107F)

    def test_tracks(self):
        self.assertEqual(converted({'tracks': '0x0000107f'}).profile.tracks, 0x107F)

    def test_mars(self):
        self.assertEqual(converted({'mars': True}).profile.mars, 1)

    def test_speeds_go_to_both_open_class_fields(self):
        profile = converted({'speeds': 3}).profile

        self.assertEqual((profile.speed_class, profile.tourney_class), (3, 3))

    def test_cheats_on_bits_5_and_6_are_stored_twice(self):
        profile = converted({'cheatsOn': '0x60'}).profile

        self.assertEqual((profile.cheats_on[5], profile.cheats_on[6], profile.infinite_shield, profile.infinite_turbo),
                         (1, 1, 1, 1))

    def test_empty_tourney_is_no_tournament_running(self):
        profile = converted({'tourney': ''}).profile

        self.assertTrue(all(record.tourney_progress == [-1] * 5 for record in profile.records))

    def test_tourney_values_go_to_character_and_level(self):
        profile = converted({'tourney': '0,1,0,0,0,0,-1,'}).profile

        self.assertEqual((profile.records[0].tourney_progress, profile.records[1].tourney_progress),
                         ([0, 1, 0, 0, 0], [0, -1, 0, 0, 0]))

    def test_trophy_bit_is_character_times_5_plus_level(self):
        profile = converted({'trophies': '000000000000000000200000'}).profile

        self.assertEqual(profile.records[4].trophies, [0, 1, 0, 0, 0])

    def test_trophy_high_word_comes_first(self):
        profile = converted({'trophies': '000080000000000000000000'}).profile

        self.assertEqual(profile.records[15].trophies, [0, 0, 0, 0, 1])

    def test_cash(self):
        profile = converted({'cash': '45206,0,7,'}).profile

        self.assertEqual([record.cash for record in profile.records[:3]], [45206, 0, 7])

    def test_upgrades_are_clamped_to_7(self):
        profile = converted({'upgrades': '2,1,0,9,'}).profile

        self.assertEqual((profile.records[0].upgrades, profile.records[1].upgrades), ([2, 1, 0], [7, 0, 0]))

    def test_tournament_high_scores_go_per_character_and_track(self):
        profile = converted({'hiTourney': '0,' * 18 + '5,'}).profile

        self.assertEqual(profile.records[1].hi_tourney[0], 5)

    def test_quick_race_high_scores_go_per_character_and_track(self):
        profile = converted({'hiQuick': '0,' * 17 + '6,'}).profile

        self.assertEqual(profile.records[0].hi_quick[17], 6)

    def test_redshift_wins(self):
        self.assertEqual(converted({'redshiftWins': '0x5'}).profile.redshift_wins, [1, 0, 1])

    def test_tourney_level(self):
        self.assertEqual(converted({'tourneyLevel': 2}).profile.tourney_level, 2)

    def test_eggs_unlocked_and_on(self):
        profile = converted({'eggs': '0x81', 'eggsOn': '0x01'}).profile

        self.assertEqual((profile.eggs, profile.eggs_on), ([1, 0, 0, 0, 0, 0, 0, 1], [1, 0, 0, 0, 0, 0, 0, 0]))

    def test_cheats_unlocked(self):
        self.assertEqual(converted({'cheats': '0x03'}).profile.cheats, [1, 1, 0, 0, 0, 0, 0])

    def test_cheat_menu(self):
        self.assertEqual(converted({'cheatMenu': True}).profile.cheat_menu, 1)

    def test_options_set_the_settings_bytes(self):
        settings = converted({'options': '5=0;6=1;8=7;15=0;16=4;11=1;12=2;13=3;14=1;'}).settings

        self.assertEqual((settings.game, settings.sound), ([0, 0, 0, 1, 1, 1, 1, 7, 0, 4], [0, 0, 0, 1, 2, 3, 1]))

    def test_option_outside_its_choices_keeps_the_default(self):
        settings = converted({'options': '15=3;'}).settings

        self.assertEqual(settings.game[8], 2)

    def test_best_laps(self):
        profile = converted({'bestLaps': '63.125,999999.000,'}).profile

        self.assertEqual(profile.lap_times()[:3], [63.125, 999999.0, 999999.0])

    def test_playlist_becomes_32_empty_slots_with_music_source_default(self):
        save = converted({'musicCustom': True, 'playlist': 'My Album\\song.wma'})

        self.assertEqual((save.songs(), save.profile.music_default), ([], 1))

    def test_missing_keys_keep_the_reset_values(self):
        save = converted({})

        self.assertEqual(save.body(), qp.new_save('PLAYER1').body())


class OriginalToViewerTests(unittest.TestCase):
    def setUp(self):
        self.document = qp.to_viewer_json(played_save())

    def test_unlock_fields(self):
        values = [self.document[key] for key in ('characters', 'tracks', 'mars', 'speeds', 'tourneyLevel')]

        self.assertEqual(values, ['0x10ff', '0x0000107f', True, 3, 2])

    def test_percent(self):
        self.assertEqual(self.document['percent'], 2 + 1 + 1 + 3)

    def test_cheat_switch_shows_as_cheats_on_bit(self):
        save = qp.new_save('PLAYER1')
        save.profile.infinite_shield = 1

        self.assertEqual(qp.to_viewer_json(save)['cheatsOn'], '0x20')

    def test_egg_and_cheat_masks(self):
        values = [self.document[key] for key in ('eggs', 'eggsOn', 'cheats', 'cheatsOn')]

        self.assertEqual(values, ['0x04', '0x04', '0x40', '0x40'])

    def test_trophies(self):
        self.assertEqual(self.document['trophies'], '%024x' % (0b11 << 20))

    def test_tourney_lists_every_character_and_level(self):
        self.assertEqual(self.document['tourney'].split(',')[20:25], ['-1', '-1', '2', '0', '-1'])

    def test_cash_and_upgrades(self):
        self.assertEqual((self.document['cash'].split(',')[4], self.document['upgrades'].split(',')[12:15]),
                         ('72564', ['4', '2', '2']))

    def test_options_string_in_viewer_order(self):
        self.assertEqual(self.document['options'], '5=1;4=1;3=1;2=1;6=0;8=4;15=2;16=0;11=4;12=4;13=7;14=0;')

    def test_best_laps_are_printed_to_the_millisecond(self):
        self.assertEqual(self.document['bestLaps'].split(',')[7], '63.123')

    def test_redshift_wins_and_music(self):
        self.assertEqual((self.document['redshiftWins'], self.document['musicCustom']), ('0x5', True))

    def test_fields_the_viewer_lacks_are_kept_under_original(self):
        extras = self.document['original']

        self.assertEqual((extras['camera'], extras['gameSpeed'], extras['hintQuickRace'], extras['unusedC84'],
                          extras['songs']), (3, 2, 30, 9, 'Song'))


class RoundTripTests(unittest.TestCase):
    def test_original_to_json_to_original_is_byte_identical(self):
        save = played_save()

        back = qp.from_viewer_json(json.loads(qp.viewer_json_text(qp.to_viewer_json(save))))

        self.assertEqual(back.body(), save.body())

    def test_viewer_json_to_original_to_json_keeps_the_values(self):
        document = fixture_document()
        written_out = {'bestLaps'}   # "" (no laps yet) comes back as the 90 values the viewer reads it as

        back = qp.to_viewer_json(qp.from_viewer_json(document))

        self.assertEqual({key: back[key] for key in document if key not in written_out},
                         {key: value for key, value in document.items() if key not in written_out})

    def test_empty_best_laps_come_back_as_no_lap(self):
        document = fixture_document()

        back = qp.to_viewer_json(qp.from_viewer_json(document))

        self.assertEqual(back['bestLaps'], '999999.000,' * 90)

    def test_new_viewer_profile_round_trips_its_values(self):
        document = {'name': 'NEW', 'options': '', 'tourney': '', 'bestLaps': ''}

        back = qp.to_viewer_json(qp.from_viewer_json(document))

        self.assertEqual(qp.from_viewer_json(back).body(), qp.from_viewer_json(document).body())


class ViewerLoaderTests(unittest.TestCase):
    """What to-json writes, read with the viewer's own rules (app_profile.cpp jsonValue: the first match)."""
    VIEWER_KEYS = ('name', 'characters', 'tracks', 'mars', 'speeds', 'redshiftWins', 'tourneyLevel', 'options', 'eggs',
                   'eggsOn', 'cheats', 'cheatsOn', 'cheatMenu', 'musicCustom', 'playlist', 'tourney', 'trophies', 'cash',
                   'upgrades', 'bestLaps', 'hiTourney', 'hiQuick')

    def test_viewer_reads_every_key_as_written(self):
        document = qp.to_viewer_json(played_save())
        text = qp.viewer_json_text(document)

        read = {key: viewer_json_value(text, key) for key in self.VIEWER_KEYS}

        expected = {key: json.dumps(document[key]) if not isinstance(document[key], str) else document[key]
                    for key in self.VIEWER_KEYS}
        self.assertEqual(read, expected)

    def test_fixture_has_the_viewer_shape(self):
        document = fixture_document()

        self.assertTrue(set(self.VIEWER_KEYS) <= set(document))


class CommandLineTests(unittest.TestCase):
    def run_main(self, arguments):
        no_keys_json = ['--keys', str(Path(tempfile.gettempdir()) / 'qr_profile_tests_no_keys.json')]
        with redirect_stdout(StringIO()), redirect_stderr(StringIO()):
            return qp.main(no_keys_json + arguments)

    def test_new_without_a_key_is_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            status = self.run_main(['new', 'PLAYER1', folder])

            self.assertEqual((status, list(Path(folder).iterdir())), (1, []))

    def test_new_unsigned_writes_the_hashed_folder(self):
        with tempfile.TemporaryDirectory() as folder:
            self.run_main(['new', 'PLAYER1', folder, '--unsigned'])

            names = sorted(path.name for path in (Path(folder) / 'B1CD4EB5DDBF').iterdir())

        self.assertEqual(names, ['SaveMeta.xbx', 'savegame.sav'])

    def test_resign_makes_verify_pass(self):
        with tempfile.TemporaryDirectory() as folder:
            key_file, xbe_file = Path(folder) / 'key.txt', Path(folder) / 'default.xbe'
            key_file.write_text(FAKE_CERT_KEY.hex(), encoding='ascii')
            xbe_file.write_bytes(fake_xbe())
            self.run_main(['new', 'PLAYER1', folder, '--exact-folder', '--unsigned'])
            keys = ['--cert-key', str(key_file), '--xbe', str(xbe_file)]
            self.run_main(keys + ['resign', folder])

            status = self.run_main(keys + ['verify', folder])

        self.assertEqual(status, 0)

    def test_win_all_edits_the_save_in_place_and_signs_it(self):
        with tempfile.TemporaryDirectory() as folder:
            key_file, xbe_file = Path(folder) / 'key.txt', Path(folder) / 'default.xbe'
            key_file.write_text(FAKE_CERT_KEY.hex(), encoding='ascii')
            xbe_file.write_bytes(fake_xbe())
            keys = ['--cert-key', str(key_file), '--xbe', str(xbe_file)]
            self.run_main(['new', 'PLAYER1', folder, '--exact-folder', '--unsigned'])
            self.run_main(keys + ['win-all', folder])

            save = qp.load_save(folder)
            result = (save.profile.percent_complete(), fake_signing_key().check(save))

        self.assertEqual(result, (96, qp.SignatureStatus.VALID))

    def test_resign_rewrites_a_stale_save_meta(self):
        with tempfile.TemporaryDirectory() as parent:
            key_file, xbe_file = Path(parent) / 'key.txt', Path(parent) / 'default.xbe'
            key_file.write_text(FAKE_CERT_KEY.hex(), encoding='ascii')
            xbe_file.write_bytes(fake_xbe())
            folder = save_folder_with_stale_meta(parent)

            self.run_main(['--cert-key', str(key_file), '--xbe', str(xbe_file), 'resign', str(folder)])

            name = qp.save_meta_name((folder / 'SaveMeta.xbx').read_bytes())
        self.assertEqual(name, 'Profile: PLAYER1')

    def test_win_all_without_a_key_is_refused(self):
        with tempfile.TemporaryDirectory() as folder:
            self.run_main(['new', 'PLAYER1', folder, '--exact-folder', '--unsigned'])
            before = (Path(folder) / 'savegame.sav').read_bytes()

            status = self.run_main(['win-all', folder])

            after = (Path(folder) / 'savegame.sav').read_bytes()
        self.assertEqual((status, after), (1, before))


if __name__ == '__main__':
    unittest.main()
