"""Quantum Redshift (Xbox, USA) profile saves: read, edit, sign and convert them to / from the Argon viewer's JSON.

A profile is a save folder E:\\UDATA\\4d530015\\<id>\\ holding SaveMeta.xbx and savegame.sav (docs\\saves\\README.md).
Signing needs either the game's own signing key (--signature-key: the kernel's XboxSignatureKey while the game runs,
read from xemu by tools\\saves\\signature_key.py) or the XboxCERTKey (--cert-key, a kernel system key) and the game's
default.xbe. Both are key files the user supplies, kept outside the repo.

usage: python tools\\saves\\profiles.py {info,verify,resign,to-json,from-json,new} ...   (-h for each)
"""
import argparse
import hashlib
import hmac
import json
import re
import struct
import sys
from dataclasses import dataclass
from enum import Enum
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'xbe'))   # tools\xbe\xbe.py, also under python -I
import xbe  # noqa: E402

SAVE_FILE = 'savegame.sav'
META_FILE = 'SaveMeta.xbx'

NAME_BYTES = 0x100
PROFILE_BYTES = 0xCC4
SLOT_BYTES = 0x48
PLAYLIST_SLOTS = 32
SIGNATURE_BYTES = 20
SAVE_BYTES = 0x16E9
SIGNED_BYTES = SAVE_BYTES - SIGNATURE_BYTES
MAX_NAME_LENGTH = 9   # FE NAME ENTRY

CHARACTERS = 16
LEVELS = 5
TRACKS = 18
SPEED_CLASSES = 5
EGGS = 8
CHEATS = 7
UPGRADES = 3
MAX_UPGRADE = 7
LAPS = TRACKS * SPEED_CLASSES
NO_LAP = 999999.0
NO_LAP_BITS = 0x497423F0   # 999999.0 as f32
INFINITE_SHIELD_CHEAT = 5
INFINITE_TURBO_CHEAT = 6
FOLDER_HASH_MODULUS = 0xFFFFFFFFFFC5
WON = 1                    # a held trophy, an unlocked egg, Mars open (0x84160 writes 1)
NO_TOURNEY = -1            # a level's progress with no tournament running
ALL_CHARACTERS = (1 << CHARACTERS) - 1
ALL_COURSE_ROUTES = 0xFFFF   # bit c every course, bit c + 8 its second route

CHARACTER_NAMES = ('Tara', 'Yuri', 'Kumiko', 'Darius', 'Jordan', 'Marcus', 'Leah', 'Ka Hekili',
                   'Alicia', 'Vladimir', 'Abi', 'Zafir', 'Kaitlyn', 'Jaya', 'Lukas', 'Kutoo')
COURSE_NAMES = ('Silver', 'Kalau', 'Fuusoku', 'Flames', 'Solar', 'Bracken', 'Mount', 'Draco')
TRACK_NAMES = COURSE_NAMES + tuple(name + ' (2nd route)' for name in COURSE_NAMES) + ('Mars', 'Mars (Terraformed)')
LEVEL_NAMES = ('Novice', 'Amateur', 'Expert', 'Master', 'Redshift')
UPGRADE_NAMES = ('Weapon Strength', 'Turbo', 'Shield')
EGG_NAMES = ('Ultra Focal Length', 'Hyper Blur', 'Atlantis Mode', 'Slide', 'Always Rain', 'Weapons Off', 'Fruity Fruit',
             'Mirror Tracks')
CHEAT_NAMES = ('Unlock all characters', 'Unlock all speeds', 'Unlock all tracks', 'Unlock all easter eggs',
               'Fully upgrade all characters', 'Infinite Shield', 'Infinite Turbo')
REDSHIFT_WIN_NAMES = ('Redshift win on Mars (Martian route)', 'Redshift win on Mars (Terraformed route)',
                      'Redshift win with Ultra Focal Length on')
REDSHIFT_WIN_POINTS = (1, 1, 2)
LATER_CHARACTERS = range(8, CHARACTERS)   # the ones Percentage Complete counts


# ---- the Game / Sound Settings lists (0x81fd0 / 0x824c0): one byte per entry, in list order

@dataclass(frozen=True)
class SettingEntry:
    type_id: int
    label: str
    choices: tuple = ()   # empty: not a choice (Exit, Reset to Default, Music Options); the byte is 0
    default: int = 0


STEPS = tuple(str(step) for step in range(8))
OFF_ON = ('Off', 'On')
EXIT = SettingEntry(0, 'Exit')
RESET = SettingEntry(1, 'Reset to Default')
SOUND_SETTINGS = (EXIT, RESET, SettingEntry(9, 'Music Options'),
                  SettingEntry(0xb, 'Engine Volume', STEPS, 4), SettingEntry(0xc, 'SFX Volume', STEPS, 4),
                  SettingEntry(0xd, 'Music Volume', STEPS, 4), SettingEntry(0xe, 'Headphones', ('No', 'Yes')))
GAME_SETTINGS = (EXIT, RESET,
                 SettingEntry(5, 'Controller 4 Vibration', OFF_ON, 1), SettingEntry(4, 'Controller 3 Vibration', OFF_ON, 1),
                 SettingEntry(3, 'Controller 2 Vibration', OFF_ON, 1), SettingEntry(2, 'Controller 1 Vibration', OFF_ON, 1),
                 SettingEntry(6, 'Speed Units', ('km/h', 'mph')), SettingEntry(8, 'Brightness', STEPS, 4),
                 SettingEntry(0xf, 'HUD', ('Off', 'Map Off', 'Full'), 2),
                 SettingEntry(0x10, 'Multiplayer Laps', ('Default', '3', '5', '7', '9')))


@dataclass
class Settings:
    sound: list   # one byte per SOUND_SETTINGS entry (the file stores the sound list first)
    game: list    # one byte per GAME_SETTINGS entry

    @classmethod
    def defaults(cls):
        return cls([entry.default for entry in SOUND_SETTINGS], [entry.default for entry in GAME_SETTINGS])

    @classmethod
    def unpack(cls, data):
        return cls(list(data[:len(SOUND_SETTINGS)]), list(data[len(SOUND_SETTINGS):]))

    def pack(self):
        return bytes(self.sound) + bytes(self.game)

    def choice_entries(self):
        """(entry, value, list, index) for every entry with choices, in the viewer's order: game list, then sound list."""
        found = []
        for entries, values in ((GAME_SETTINGS, self.game), (SOUND_SETTINGS, self.sound)):
            for index, entry in enumerate(entries):
                if entry.choices:
                    found.append((entry, values[index], values, index))
        return found

    def set_choice(self, type_id, value):
        for entry, _, values, index in self.choice_entries():
            if entry.type_id == type_id:
                values[index] = value


SETTINGS_BYTES = len(SOUND_SETTINGS) + len(GAME_SETTINGS)


# ---- savegame.sav's blocks

PROFILE_HEAD = struct.Struct('<%dIiH2si' % LAPS)
CHARACTER_RECORD = struct.Struct('<i3s5s5i18i18i')
PROFILE_TAIL = struct.Struct('<iiBB2siiIB3si8s8s7s7sBB3i3ss')
PLAYLIST_SLOT = struct.Struct('<ii64s')
FLOAT32 = struct.Struct('<f')
UINT32 = struct.Struct('<I')


def float32_bits(value):
    return UINT32.unpack(FLOAT32.pack(value))[0]


def float32_value(bits):
    return FLOAT32.unpack(UINT32.pack(bits))[0]


@dataclass
class CharacterRecord:
    """Profile +0x174 + character x 0xb0."""
    cash: int
    upgrades: list           # 0..7: Weapon Strength, Turbo, Shield
    trophies: list           # one byte per tournament level, non-zero: held
    tourney_progress: list   # races won per level; -1: no tournament running
    hi_tourney: list         # tournament high score per track
    hi_quick: list           # Quick Race high score per track

    @classmethod
    def reset(cls):
        return cls(0, [0] * UPGRADES, [0] * LEVELS, [NO_TOURNEY] * LEVELS, [0] * TRACKS, [0] * TRACKS)

    @classmethod
    def unpack(cls, data, offset):
        fields = CHARACTER_RECORD.unpack_from(data, offset)
        return cls(fields[0], list(fields[1]), list(fields[2]), list(fields[3:8]), list(fields[8:26]), list(fields[26:44]))

    def pack(self):
        return CHARACTER_RECORD.pack(self.cash, bytes(self.upgrades), bytes(self.trophies), *self.tourney_progress,
                                     *self.hi_tourney, *self.hi_quick)

    def trophy_level(self):
        """0x82910: the highest level whose trophy is held, + 1; 0 without any."""
        held = [level + 1 for level in range(LEVELS) if self.trophies[level]]
        return max(held, default=0)


@dataclass
class ProfileBlock:
    """g_profile 0x1baf74..0x1bbc37, stored raw at file offset 0x100."""
    best_lap_bits: list     # f32 bits per track x 5 + speed (kept as bits: exact, NaNs included); NO_LAP: none
    camera: int             # race camera view
    characters: int         # unlocked, bit per character
    pad_16e: bytes
    speed_class: int        # highest open speed class 0..4
    records: list           # CharacterRecord per character
    game_speed: int         # the game speed last chosen
    tourney_level: int      # the tournament level last started
    infinite_shield: int    # cheat 5's switch
    infinite_turbo: int     # cheat 6's switch
    pad_c7e: bytes
    unused_c80: int
    unused_c84: int
    tracks: int             # unlocked: bit i track i, bit i + 8 its second route
    mars: int
    pad_c8d: bytes
    tourney_class: int      # highest open tournament level, set together with speed_class
    eggs: list              # easter egg unlocked, byte per egg
    eggs_on: list           # easter egg switched on
    cheats: list            # cheat unlocked by its code
    cheats_on: list         # cheat activated
    cheat_menu: int         # the name started "CHEAT"
    music_default: int      # Music Source: 1 Default, 0 Custom
    hints: list             # loading hint index: Time Attack, Quick Race, Tournament
    redshift_wins: list     # byte per REDSHIFT_WIN_NAMES
    pad_cc3: bytes

    @classmethod
    def reset(cls):
        """profile_reset 0x83b60."""
        return cls(best_lap_bits=[NO_LAP_BITS] * LAPS, camera=1, characters=0x7f, pad_16e=bytes(2), speed_class=1,
                   records=[CharacterRecord.reset() for _ in range(CHARACTERS)], game_speed=0, tourney_level=0,
                   infinite_shield=0, infinite_turbo=0, pad_c7e=bytes(2), unused_c80=0, unused_c84=0, tracks=0x7f,
                   mars=0, pad_c8d=bytes(3), tourney_class=1, eggs=[0] * EGGS, eggs_on=[0] * EGGS, cheats=[0] * CHEATS,
                   cheats_on=[0] * CHEATS, cheat_menu=0, music_default=1, hints=[0, 0, 0], redshift_wins=[0, 0, 0],
                   pad_cc3=bytes(1))

    @classmethod
    def unpack(cls, data):
        head = PROFILE_HEAD.unpack_from(data, 0)
        records = [CharacterRecord.unpack(data, PROFILE_HEAD.size + c * CHARACTER_RECORD.size) for c in range(CHARACTERS)]
        tail = PROFILE_TAIL.unpack_from(data, PROFILE_HEAD.size + CHARACTERS * CHARACTER_RECORD.size)
        return cls(best_lap_bits=list(head[:LAPS]), camera=head[LAPS], characters=head[LAPS + 1], pad_16e=head[LAPS + 2],
                   speed_class=head[LAPS + 3], records=records, game_speed=tail[0], tourney_level=tail[1],
                   infinite_shield=tail[2], infinite_turbo=tail[3], pad_c7e=tail[4], unused_c80=tail[5],
                   unused_c84=tail[6], tracks=tail[7], mars=tail[8], pad_c8d=tail[9], tourney_class=tail[10],
                   eggs=list(tail[11]), eggs_on=list(tail[12]), cheats=list(tail[13]), cheats_on=list(tail[14]),
                   cheat_menu=tail[15], music_default=tail[16], hints=list(tail[17:20]), redshift_wins=list(tail[20]),
                   pad_cc3=tail[21])

    def pack(self):
        head = PROFILE_HEAD.pack(*self.best_lap_bits, self.camera, self.characters, self.pad_16e, self.speed_class)
        records = b''.join(record.pack() for record in self.records)
        tail = PROFILE_TAIL.pack(self.game_speed, self.tourney_level, self.infinite_shield, self.infinite_turbo,
                                 self.pad_c7e, self.unused_c80, self.unused_c84, self.tracks, self.mars, self.pad_c8d,
                                 self.tourney_class, bytes(self.eggs), bytes(self.eggs_on), bytes(self.cheats),
                                 bytes(self.cheats_on), self.cheat_menu, self.music_default, *self.hints,
                                 bytes(self.redshift_wins), self.pad_cc3)
        return head + records + tail

    def lap_times(self):
        return [float32_value(bits) for bits in self.best_lap_bits]

    def set_open_class(self, speed_class):
        """The game opens a speed class and the tournament level together (+0x170, +0xc90)."""
        self.speed_class = speed_class
        self.tourney_class = speed_class

    def win_all_tournaments(self):
        """Every character's tournament won at every level, with what profile_tournament_won 0x84160 opens on the way:
        each trophy (1), every character (the nemeses, Ka Hekili, Kutoo), both routes of every course (bit c + 8 per
        winner, Draco), Mars, the Redshift class, every easter egg (1); no tournament left running (-1)."""
        for record in self.records:
            record.trophies = [WON] * LEVELS
            record.tourney_progress = [NO_TOURNEY] * LEVELS
        self.characters = ALL_CHARACTERS
        self.tracks = ALL_COURSE_ROUTES
        self.mars = WON
        self.set_open_class(len(LEVEL_NAMES) - 1)
        self.eggs = [WON] * EGGS

    def percent_complete(self):
        """0xc03a0: trophy levels + later characters + eggs + the Redshift wins (100 in all)."""
        trophies = sum(record.trophy_level() for record in self.records)
        characters = sum(1 for c in LATER_CHARACTERS if self.characters >> c & 1)
        eggs = sum(1 for egg in self.eggs if egg)
        wins = sum(points for points, won in zip(REDSHIFT_WIN_POINTS, self.redshift_wins) if won)
        return trophies + characters + eggs + wins


@dataclass
class PlaylistSlot:
    song: int
    soundtrack: int
    name_bytes: bytes   # wchar[32]; stale stack data in an empty slot

    @classmethod
    def empty(cls):
        return cls(-1, -1, bytes(0x40))

    @classmethod
    def unpack(cls, data, offset):
        return cls(*PLAYLIST_SLOT.unpack_from(data, offset))

    def pack(self):
        return PLAYLIST_SLOT.pack(self.song, self.soundtrack, self.name_bytes)

    def is_empty(self):
        return self.song < 0 or self.soundtrack < 0

    def song_name(self):
        return decode_wide_string(self.name_bytes)[0]


def decode_wide_string(field):
    """(text up to the NUL, the bytes after the NUL)."""
    for offset in range(0, len(field) - 1, 2):
        if field[offset:offset + 2] == b'\0\0':
            return field[:offset].decode('utf-16-le', 'surrogatepass'), field[offset + 2:]
    return field.decode('utf-16-le', 'surrogatepass'), b''


def encode_name_field(name, padding):
    """The name, its NUL, then the kept padding when it still fits exactly, else zeros."""
    text = name.encode('utf-16-le', 'surrogatepass')
    if len(text) < NAME_BYTES:
        text += b'\0\0'
    rest = NAME_BYTES - len(text)
    if rest < 0:
        raise ValueError('profile name %r is longer than the 127 characters the save holds' % name)
    return text + (padding if len(padding) == rest else bytes(rest))


def validate_name(name):
    if not name or '\0' in name:
        raise ValueError('a profile name needs 1 to %d characters' % MAX_NAME_LENGTH)
    if len(name) > MAX_NAME_LENGTH:
        raise ValueError('profile name %r is longer than the game\'s %d characters' % (name, MAX_NAME_LENGTH))


@dataclass
class SaveGame:
    """savegame.sav: name, profile block, settings, playlist, signature (exactly SAVE_BYTES)."""
    name: str
    name_padding: bytes   # the bytes after the name's NUL (the game leaves stack data there)
    profile: ProfileBlock
    settings: Settings
    playlist: list        # PlaylistSlot x 32
    signature: bytes

    @classmethod
    def parse(cls, data):
        if len(data) != SAVE_BYTES:
            raise ValueError('savegame.sav must be %d bytes, this one has %d' % (SAVE_BYTES, len(data)))
        name, padding = decode_wide_string(data[:NAME_BYTES])
        profile_end = NAME_BYTES + PROFILE_BYTES
        settings_end = profile_end + SETTINGS_BYTES
        playlist = [PlaylistSlot.unpack(data, settings_end + i * SLOT_BYTES) for i in range(PLAYLIST_SLOTS)]
        return cls(name, padding, ProfileBlock.unpack(data[NAME_BYTES:profile_end]),
                   Settings.unpack(data[profile_end:settings_end]), playlist, data[SIGNED_BYTES:])

    def body(self):
        """Every byte the signature covers."""
        return (encode_name_field(self.name, self.name_padding) + self.profile.pack() + self.settings.pack()
                + b''.join(slot.pack() for slot in self.playlist))

    def to_bytes(self):
        return self.body() + self.signature

    def rename(self, name):
        validate_name(name)
        self.name = name
        self.name_padding = b''

    def sign(self, signing_key):
        self.signature = signing_key.signature(self.body())

    def songs(self):
        return [slot.song_name() for slot in self.playlist if not slot.is_empty()]


def new_save(name):
    """A fresh profile as FE NAME ENTRY saves it: profile_reset 0x83b60 (and 0x83af0 for a name starting "CHEAT")."""
    validate_name(name)
    profile = ProfileBlock.reset()
    profile.cheat_menu = int(name.startswith('CHEAT'))
    return SaveGame(name, b'', profile, Settings.defaults(), [PlaylistSlot.empty() for _ in range(PLAYLIST_SLOTS)],
                    bytes(SIGNATURE_BYTES))


# ---- signing: the XAPI roamable signature, HMAC-SHA1 keyed with HMAC-SHA1(XboxCERTKey, certificate key)[:16]

class SignatureStatus(Enum):
    VALID = 'valid'
    INVALID = 'invalid (the game lists it as "Invalid profile")'
    NOT_CHECKED = 'not checked (no key: no key file given, none in keys.json)'


class SigningKeyMissing(Exception):
    pass


class SigningKey:
    available = True

    def __init__(self, key):
        self._key = key

    def __repr__(self):
        return 'SigningKey(<hidden>)'

    def signature(self, body):
        return hmac.new(self._key, body, hashlib.sha1).digest()

    def check(self, save):
        signed = hmac.compare_digest(self.signature(save.body()), save.signature)
        return SignatureStatus.VALID if signed else SignatureStatus.INVALID


class MissingSigningKey:
    available = False

    def __repr__(self):
        return 'MissingSigningKey()'

    def signature(self, body):
        raise SigningKeyMissing('signing needs a key (--signature-key, --cert-key or keys.json)')

    def check(self, save):
        return SignatureStatus.NOT_CHECKED


KEYS_JSON = Path(__file__).resolve().parents[2] / 'keys.json'   # the user's keys, kept out of git (.gitignore)
GAME_KEY_NAME, CERT_KEY_NAME, CERTIFICATE_KEY_NAME = 'XboxSignatureKey', 'XboxCERTKey', 'XbeSignature'


def parse_key_digits(text, source):
    """16 bytes as 32 hex digits, whitespace allowed. The error never shows the text."""
    digits = ''.join(text.split())
    if not re.fullmatch('[0-9A-Fa-f]{32}', digits):
        raise ValueError('%s: the key must be 16 bytes as 32 hex digits' % source)
    return bytes.fromhex(digits)


def read_key_file(path):
    if str(path).lower().endswith('.json'):
        raise ValueError('%s is a keys.json, not a key file: pass it with --keys' % path)
    return parse_key_digits(Path(path).read_text(encoding='ascii', errors='replace'), path)


def read_keys_json(path):
    """keys.json's keys as bytes by name; a missing file or an empty entry gives nothing for it."""
    if not path or not Path(path).is_file():
        return {}
    document = json.loads(Path(path).read_text(encoding='utf-8'))
    names = (GAME_KEY_NAME, CERT_KEY_NAME, CERTIFICATE_KEY_NAME)
    return {name: parse_key_digits(document[name], '%s %s' % (path, name)) for name in names if document.get(name)}


def derive_signing_key(cert_key, certificate_key):
    return SigningKey(hmac.new(cert_key, certificate_key, hashlib.sha1).digest()[:16])


def default_xbe_path():
    return Path(__file__).resolve().parents[2] / 'game_data' / 'default.xbe'


@dataclass
class KeyFiles:
    """Where the signing key comes from, first found: the game's own key file, the XboxCERTKey file, then keys.json's
    XboxSignatureKey, then its XboxCERTKey. The XboxCERTKey goes with the certificate key of --xbe, else keys.json's
    XbeSignature, else default.xbe's."""
    signature_key: str = None
    cert_key: str = None
    xbe: str = None
    keys_json: str = str(KEYS_JSON)


def certificate_key_of(files, keys):
    if not files.xbe and CERTIFICATE_KEY_NAME in keys:
        return keys[CERTIFICATE_KEY_NAME]
    xbe_path = Path(files.xbe) if files.xbe else default_xbe_path()
    if not xbe_path.is_file():
        raise ValueError('%s not found: signing with the XboxCERTKey needs the game\'s default.xbe (--xbe)' % xbe_path)
    return xbe.certificate_signature_key(xbe_path.read_bytes())


def chosen_signing_key(files):
    """(where the key comes from, as text without the key; the key), the first source found in KeyFiles' order."""
    keys = read_keys_json(files.keys_json)
    if files.signature_key:
        return 'game key file %s' % files.signature_key, SigningKey(read_key_file(files.signature_key))
    if files.cert_key:
        cert_key = read_key_file(files.cert_key)
        return 'XboxCERTKey file %s' % files.cert_key, derive_signing_key(cert_key, certificate_key_of(files, keys))
    if GAME_KEY_NAME in keys:
        return '%s (%s)' % (files.keys_json, GAME_KEY_NAME), SigningKey(keys[GAME_KEY_NAME])
    if CERT_KEY_NAME in keys:
        key = derive_signing_key(keys[CERT_KEY_NAME], certificate_key_of(files, keys))
        return '%s (%s)' % (files.keys_json, CERT_KEY_NAME), key
    return 'no key', MissingSigningKey()


def load_signing_key(files):
    return chosen_signing_key(files)[1]


# ---- save folders: <id>\SaveMeta.xbx + savegame.sav

def save_folder_id(name):
    """XCreateSaveGame's save ID (0xedaa6): 12 hex digits hashed from the name's UTF-16 code units."""
    hashed = 0
    for (unit,) in struct.iter_unpack('<H', name.encode('utf-16-le', 'surrogatepass')):
        hashed = (hashed * 0x10000 + unit) % FOLDER_HASH_MODULUS
    return '%012X' % hashed


PROFILE_SAVE_PREFIX = 'Profile:'   # infotext 238 in English (Japanese: プレイヤー:); the dashboard language picks it


def profile_save_name(profile_name, prefix=PROFILE_SAVE_PREFIX):
    """The name the game gives a profile's save (0x3a5b1: swprintf("%s %s", infotext 238, the name), 0x459a90),
    which XCreateSaveGame hashes into the folder ID and writes as SaveMeta.xbx's Name: "Profile: PLAYER1"."""
    return '%s %s' % (prefix, profile_name)


def profile_folder_id(profile_name):
    return save_folder_id(profile_save_name(profile_name))


def save_meta_bytes(save_name):
    """SaveMeta.xbx: UTF-16LE, a BOM, "Name=<the save's name>\\r\\n", no terminator."""
    return ('﻿Name=' + save_name + '\r\n').encode('utf-16-le', 'surrogatepass')


def save_meta_name(data):
    text = data.decode('utf-16-le', 'surrogatepass').lstrip('﻿')
    for line in text.split('\r\n'):
        if line.startswith('Name='):
            return line[len('Name='):]
    raise ValueError('SaveMeta.xbx holds no Name= line')


def save_file_path(path):
    """A save folder's savegame.sav, or the path itself when it names the file."""
    path = Path(path)
    return path / SAVE_FILE if path.is_dir() else path


def load_save(path):
    file = save_file_path(path)
    try:
        return SaveGame.parse(file.read_bytes())
    except ValueError as error:
        raise ValueError('%s: %s' % (file, error)) from error


def folder_warnings(path, save):
    """What does not match in a save folder: SaveMeta's Name against the profile's save name, and the folder against
    the ID the game hashes from that name (it opens the save by it)."""
    folder = save_file_path(path).parent
    meta = folder / META_FILE
    if not meta.is_file():
        return ['%s is missing' % meta]
    warnings = []
    meta_name, expected = save_meta_name(meta.read_bytes()), profile_save_name(save.name)
    if meta_name != expected:
        warnings.append('SaveMeta Name %r differs from %r, the save name of profile %r' % (meta_name, expected, save.name))
    if folder.name.upper() != profile_folder_id(save.name):
        warnings.append('the folder is named %s, the game looks for %s' % (folder.name, profile_folder_id(save.name)))
    return warnings


def write_save_folder(save, folder, previous_name=None):
    """Writes SaveMeta.xbx (Name = the profile's save name) and savegame.sav. An existing folder must hold this
    profile: its SaveMeta Name is this profile's save name, or previous_name's when the profile is being renamed.
    Returns what does not match the game's lookup (a folder not named by the hash, as --exact-folder can write)."""
    folder = Path(folder)
    meta = folder / META_FILE
    if meta.is_file():
        owner = save_meta_name(meta.read_bytes())
        owners = {profile_save_name(save.name)} | ({profile_save_name(previous_name)} if previous_name else set())
        if owner not in owners:
            raise ValueError('%s belongs to %r, not to profile %r' % (folder, owner, save.name))
    folder.mkdir(parents=True, exist_ok=True)
    write_save_meta(folder, save)
    (folder / SAVE_FILE).write_bytes(save.to_bytes())
    return folder_warnings(folder, save)


def rewrite_save(path, save):
    """In place: savegame.sav and, in a save folder, SaveMeta.xbx with the save's name (a missing or stale one is
    written again). A savegame.sav given on its own outside a save folder is written alone. Returns what still does
    not match, as write_save_folder."""
    file = save_file_path(path)
    file.write_bytes(save.to_bytes())
    if not ((file.parent / META_FILE).is_file() or Path(path).is_dir()):
        return []
    write_save_meta(file.parent, save)
    return folder_warnings(file.parent, save)


def remove_save_folder(folder):
    """Deletes a save folder's SaveMeta.xbx and savegame.sav, and the folder when nothing else is left in it."""
    folder = Path(folder)
    for name in (META_FILE, SAVE_FILE):
        (folder / name).unlink(missing_ok=True)
    if not any(folder.iterdir()):
        folder.rmdir()


def write_save_meta(folder, save):
    """SaveMeta.xbx with the profile's save name, checked by reading it back."""
    meta = Path(folder) / META_FILE
    save_name = profile_save_name(save.name)
    meta.write_bytes(save_meta_bytes(save_name))
    if save_meta_name(meta.read_bytes()) != save_name:
        raise ValueError('%s does not read back as Name=%r' % (meta, save_name))


# ---- the viewer's profile JSON (src\app\app_profile.cpp)

def flags_to_mask(flags):
    return sum(1 << i for i, flag in enumerate(flags) if flag)


def mask_to_flags(mask, count):
    return [mask >> i & 1 for i in range(count)]


def comma_list(values):
    return ''.join('%d,' % value for value in values)


def format_lap(time):
    return '%.3f' % time




def cheats_on_mask(profile):
    """Cheats 5 / 6 are stored twice: the activated byte and the switch (+0xc7c / +0xc7d)."""
    mask = flags_to_mask(profile.cheats_on)
    mask |= (1 << INFINITE_SHIELD_CHEAT) if profile.infinite_shield else 0
    mask |= (1 << INFINITE_TURBO_CHEAT) if profile.infinite_turbo else 0
    return mask


def trophy_hex(records):
    """80 bits (character x 5 + level) as "%08x%08x%08x" of words 2, 1, 0."""
    bits = 0
    for c, record in enumerate(records):
        for level, held in enumerate(record.trophies):
            bits |= (1 << (c * LEVELS + level)) if held else 0
    return '%024x' % bits


def options_string(settings):
    return ''.join('%d=%d;' % (entry.type_id, value) for entry, value, _, _ in settings.choice_entries())


def original_extras(save):
    """What the viewer's JSON has no field for; the viewer's loader ignores this object (and drops it when it saves)."""
    profile = save.profile
    return {
        'camera': profile.camera,
        'gameSpeed': profile.game_speed,
        'hintTimeAttack': profile.hints[0],
        'hintQuickRace': profile.hints[1],
        'hintTournament': profile.hints[2],
        'unusedC80': profile.unused_c80,
        'unusedC84': profile.unused_c84,
        'bestLapsExact': b''.join(UINT32.pack(bits) for bits in profile.best_lap_bits).hex(),
        'playlistSlots': b''.join(slot.pack() for slot in save.playlist).hex(),
        'songs': '|'.join(save.songs()),
    }


def to_viewer_json(save):
    """The viewer's profile object, in the order app_profile.cpp writes it, plus the "original" extras."""
    profile = save.profile
    records = profile.records
    return {
        'name': save.name,
        'characters': '0x%04x' % profile.characters,
        'tracks': '0x%08x' % profile.tracks,
        'mars': bool(profile.mars),
        'speeds': profile.speed_class,
        'percent': profile.percent_complete(),
        'options': options_string(save.settings),
        'eggs': '0x%02x' % flags_to_mask(profile.eggs),
        'eggsOn': '0x%02x' % flags_to_mask(profile.eggs_on),
        'cheats': '0x%02x' % flags_to_mask(profile.cheats),
        'cheatsOn': '0x%02x' % cheats_on_mask(profile),
        'cheatMenu': bool(profile.cheat_menu),
        'musicCustom': not profile.music_default,
        'playlist': '',
        'tourney': comma_list(value for record in records for value in record.tourney_progress),
        'trophies': trophy_hex(records),
        'cash': comma_list(record.cash for record in records),
        'upgrades': comma_list(value for record in records for value in record.upgrades),
        'hiTourney': comma_list(value for record in records for value in record.hi_tourney),
        'hiQuick': comma_list(value for record in records for value in record.hi_quick),
        'redshiftWins': '0x%x' % flags_to_mask(profile.redshift_wins),
        'tourneyLevel': profile.tourney_level,
        'bestLaps': ''.join(format_lap(time) + ',' for time in profile.lap_times()),
        'original': original_extras(save),
    }


def viewer_json_text(document):
    return json.dumps(document, indent=2, ensure_ascii=False) + '\n'


C_INT = re.compile(r'\s*[+-]?\d+')
C_FLOAT = re.compile(r'\s*[+-]?(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?')
INT32_RANGE = (-0x80000000, 0x7FFFFFFF)


def c_atoi(text):
    found = C_INT.match(text)
    return min(max(int(found.group()), INT32_RANGE[0]), INT32_RANGE[1]) if found else 0


def c_atof(text):
    found = C_FLOAT.match(text)
    return float(found.group()) if found else 0.0


def viewer_list_items(text):
    """The items the viewer's comma-list loops read: up to each ',' (a trailing ',' ends the list)."""
    items = []
    position = 0
    while position < len(text):
        end = text.find(',', position)
        end = len(text) if end < 0 else end
        items.append(text[position:end])
        position = end + 1
    return items


def viewer_int_list(text, count, missing):
    values = [c_atoi(item) for item in viewer_list_items(text)[:count]]
    return values + [missing] * (count - len(values))


def clamp(value, low, high):
    return min(max(value, low), high)


class ViewerProfile:
    """Reads a viewer JSON object the way app_profile.cpp's loader does: missing keys keep the profile reset values."""

    def __init__(self, document):
        self._document = document

    def text(self, key):
        value = self._document.get(key, '')
        if isinstance(value, bool):
            return 'true' if value else 'false'
        return str(value)

    def has(self, key):
        return self.text(key) != ''

    def flag(self, key):
        return self.text(key) == 'true'

    def unsigned(self, key, bits):
        try:
            return int(self.text(key).strip(), 0) & ((1 << bits) - 1)
        except ValueError as error:
            raise ValueError('profile JSON: "%s" is not a number: %r' % (key, self.text(key))) from error

    def integer(self, key):
        return c_atoi(self.text(key))

    def extras(self):
        extras = self._document.get('original', {})
        return extras if isinstance(extras, dict) else {}


def from_viewer_json(document):
    """A savegame.sav from the viewer's profile object (the reset values for what it does not hold)."""
    viewer = ViewerProfile(document)
    save = new_save(viewer.text('name'))
    apply_viewer_unlocks(viewer, save.profile)
    apply_viewer_switches(viewer, save.profile)
    apply_viewer_characters(viewer, save.profile.records)
    apply_viewer_laps(viewer, save.profile)
    save.settings = settings_from_options(viewer.text('options'))
    apply_original_extras(viewer.extras(), save)
    save.profile.music_default = 0 if viewer.flag('musicCustom') and save.songs() else 1
    return save


def apply_viewer_unlocks(viewer, profile):
    if viewer.has('characters'):
        profile.characters = viewer.unsigned('characters', 16)
    if viewer.has('tracks'):
        profile.tracks = viewer.unsigned('tracks', 32)
    if viewer.has('speeds'):
        profile.set_open_class(clamp(viewer.integer('speeds'), 0, SPEED_CLASSES - 1))
    if viewer.has('redshiftWins'):
        profile.redshift_wins = mask_to_flags(viewer.unsigned('redshiftWins', 32), len(REDSHIFT_WIN_NAMES))
    if viewer.has('tourneyLevel'):
        profile.tourney_level = clamp(viewer.integer('tourneyLevel'), 0, LEVELS - 1)
    profile.mars = int(viewer.flag('mars'))


def apply_viewer_switches(viewer, profile):
    profile.eggs = mask_to_flags(viewer.unsigned('eggs', 32) if viewer.has('eggs') else 0, EGGS)
    profile.eggs_on = mask_to_flags(viewer.unsigned('eggsOn', 32) if viewer.has('eggsOn') else 0, EGGS)
    profile.cheats = mask_to_flags(viewer.unsigned('cheats', 32) if viewer.has('cheats') else 0, CHEATS)
    profile.cheats_on = mask_to_flags(viewer.unsigned('cheatsOn', 32) if viewer.has('cheatsOn') else 0, CHEATS)
    profile.infinite_shield = profile.cheats_on[INFINITE_SHIELD_CHEAT]
    profile.infinite_turbo = profile.cheats_on[INFINITE_TURBO_CHEAT]
    profile.cheat_menu = int(viewer.flag('cheatMenu'))


def apply_viewer_characters(viewer, records):
    """An empty "tourney" (a new viewer profile) is the game's -1: no tournament running."""
    progress = viewer_int_list(viewer.text('tourney'), CHARACTERS * LEVELS, 0) if viewer.has('tourney') \
        else [-1] * (CHARACTERS * LEVELS)
    trophies = trophy_bits(viewer.text('trophies'))
    cash = viewer_int_list(viewer.text('cash'), CHARACTERS, 0)
    upgrades = [clamp(value, 0, MAX_UPGRADE) for value in viewer_int_list(viewer.text('upgrades'), CHARACTERS * UPGRADES, 0)]
    hi_tourney = viewer_int_list(viewer.text('hiTourney'), CHARACTERS * TRACKS, 0)
    hi_quick = viewer_int_list(viewer.text('hiQuick'), CHARACTERS * TRACKS, 0)
    for c, record in enumerate(records):
        record.cash = cash[c]
        record.upgrades = upgrades[c * UPGRADES:(c + 1) * UPGRADES]
        record.trophies = mask_to_flags(trophies >> (c * LEVELS), LEVELS)
        record.tourney_progress = progress[c * LEVELS:(c + 1) * LEVELS]
        record.hi_tourney = hi_tourney[c * TRACKS:(c + 1) * TRACKS]
        record.hi_quick = hi_quick[c * TRACKS:(c + 1) * TRACKS]


def trophy_bits(text):
    """The viewer reads only a 24-digit "trophies"; anything else is no trophies."""
    return int(text, 16) if re.fullmatch('[0-9A-Fa-f]{24}', text) else 0


def apply_viewer_laps(viewer, profile):
    bits = [float32_bits(c_atof(item)) for item in viewer_list_items(viewer.text('bestLaps'))[:LAPS]]
    profile.best_lap_bits = bits + [NO_LAP_BITS] * (LAPS - len(bits))


def settings_from_options(text):
    """The viewer's "options" ("type=choice;"): a value outside the entry's choices keeps its default."""
    settings = Settings.defaults()
    choice_counts = {entry.type_id: len(entry.choices) for entry, _, _, _ in settings.choice_entries()}
    for pair in re.finditer(r'([^=;]*)=([^;]*);', text):
        type_id, value = c_atoi(pair.group(1)), c_atoi(pair.group(2))
        if 0 <= value < choice_counts.get(type_id, 0):
            settings.set_choice(type_id, value)
    return settings


def apply_original_extras(extras, save):
    """The fields only an original save holds, kept from a JSON that to-json made. The exact best laps are used where
    they still print as the JSON's (the JSON keeps 1 ms)."""
    profile = save.profile
    profile.camera = int(extras.get('camera', profile.camera))
    profile.game_speed = int(extras.get('gameSpeed', profile.game_speed))
    profile.hints = [int(extras.get(key, value)) for key, value in
                     zip(('hintTimeAttack', 'hintQuickRace', 'hintTournament'), profile.hints)]
    profile.unused_c80 = int(extras.get('unusedC80', profile.unused_c80))
    profile.unused_c84 = int(extras.get('unusedC84', profile.unused_c84))
    if 'bestLapsExact' in extras:
        profile.best_lap_bits = exact_laps(extras_bytes(extras, 'bestLapsExact', LAPS * UINT32.size), profile.best_lap_bits)
    if 'playlistSlots' in extras:
        slots = extras_bytes(extras, 'playlistSlots', PLAYLIST_SLOTS * SLOT_BYTES)
        save.playlist = [PlaylistSlot.unpack(slots, i * SLOT_BYTES) for i in range(PLAYLIST_SLOTS)]


def extras_bytes(extras, key, size):
    try:
        data = bytes.fromhex(extras[key])
    except (TypeError, ValueError) as error:
        raise ValueError('profile JSON: original.%s is not hex' % key) from error
    if len(data) != size:
        raise ValueError('profile JSON: original.%s must be %d bytes, it has %d' % (key, size, len(data)))
    return data


def exact_laps(data, json_bits):
    exact = [bits for (bits,) in UINT32.iter_unpack(data)]
    return [old if format_lap(float32_value(old)) == format_lap(float32_value(new)) else new
            for old, new in zip(exact, json_bits)]


def load_viewer_json(path):
    try:
        document = json.loads(Path(path).read_text(encoding='utf-8'))
    except json.JSONDecodeError as error:
        raise ValueError('%s: not valid JSON (%s)' % (path, error)) from error
    if not isinstance(document, dict):
        raise ValueError('%s: a profile JSON is one object' % path)
    return document


# ---- the command line

def describe_bits(mask, names):
    named = [name for i, name in enumerate(names) if mask >> i & 1]
    return ', '.join(named) if named else 'none'


def describe_settings(settings):
    return ', '.join('%s %s' % (entry.label, entry.choices[value] if value < len(entry.choices) else value)
                     for entry, value, _, _ in settings.choice_entries())


def describe_character(c, record):
    upgrades = '/'.join(str(level) for level in record.upgrades)
    trophies = ''.join(LEVEL_NAMES[level][0] if held else '-' for level, held in enumerate(record.trophies))
    progress = ','.join(str(value) for value in record.tourney_progress)
    return '  %-10s cash %8d  upgrades %s  trophies %s  progress %s' % (
        CHARACTER_NAMES[c], record.cash, upgrades, trophies, progress)


def describe_save(save, status):
    profile = save.profile
    lines = [
        'Name:              %s' % save.name,
        'Save folder:       %s (%s)' % (profile_folder_id(save.name), profile_save_name(save.name)),
        'Percent complete:  %d%%' % profile.percent_complete(),
        'Signature:         %s' % status.value,
        'Characters:        %s' % describe_bits(profile.characters, CHARACTER_NAMES),
        'Tracks:            %s' % describe_bits(profile.tracks, TRACK_NAMES[:16]),
        'Mars:              %s' % ('unlocked' if profile.mars else 'locked'),
        'Speed class open:  %d (%s); tournament level open %d' % (
            profile.speed_class, LEVEL_NAMES[clamp(profile.speed_class, 0, 4)], profile.tourney_class),
        'Last tournament:   %s' % LEVEL_NAMES[clamp(profile.tourney_level, 0, 4)],
        'Easter eggs:       %s; on: %s' % (describe_bits(flags_to_mask(profile.eggs), EGG_NAMES),
                                          describe_bits(flags_to_mask(profile.eggs_on), EGG_NAMES)),
        'Cheats:            %s; on: %s; cheat menu %s' % (
            describe_bits(flags_to_mask(profile.cheats), CHEAT_NAMES), describe_bits(cheats_on_mask(profile), CHEAT_NAMES),
            'shown' if profile.cheat_menu else 'hidden'),
        'Redshift wins:     %s' % describe_bits(flags_to_mask(profile.redshift_wins), REDSHIFT_WIN_NAMES),
        'Music source:      %s; playlist: %s' % ('Default' if profile.music_default else 'Custom',
                                                ' | '.join(save.songs()) or 'empty'),
        'Settings:          %s' % describe_settings(save.settings),
        'Camera %d, game speed %d, loading hints %s' % (profile.camera, profile.game_speed,
                                                         '/'.join(str(hint) for hint in profile.hints)),
        'Per character (trophies N/A/E/M/R, tournament progress -1 = none running):',
    ]
    lines += [describe_character(c, record) for c, record in enumerate(save.profile.records)]
    return '\n'.join(lines)


def warn(message):
    print('warning: ' + message, file=sys.stderr)


def sign_or_keep(save, signing_key, unsigned_allowed):
    """Signs with the key; without one the save keeps its signature (zeros for a new one), only with --unsigned."""
    if signing_key.available:
        save.sign(signing_key)
        return
    if not unsigned_allowed:
        raise SigningKeyMissing('no key (--signature-key, --cert-key or keys.json): pass --unsigned to write it anyway '
                                '(the game will list it as "Invalid profile")')
    warn('no signing key: savegame.sav keeps a stale signature, the game will list it as "Invalid profile"')


def output_folder(save, out, exact_folder):
    return Path(out) if exact_folder else Path(out) / profile_folder_id(save.name)


def run_info(args, signing_key):
    save = load_save(args.save)
    print(describe_save(save, signing_key.check(save)))
    if Path(args.save).is_dir():
        warn_all(folder_warnings(args.save, save))
    return 0


def run_verify(args, signing_key):
    status = signing_key.check(load_save(args.save))
    print('%s: signature %s' % (save_file_path(args.save), status.value))
    return {SignatureStatus.VALID: 0, SignatureStatus.INVALID: 1, SignatureStatus.NOT_CHECKED: 2}[status]


def warn_all(messages):
    for message in messages:
        warn(message)


def run_resign(args, signing_key):
    save = load_save(args.save)
    save.sign(signing_key)
    warn_all(rewrite_save(args.save, save))
    print('%s: signed' % save_file_path(args.save))
    return 0


def run_win_all(args, signing_key):
    save = load_save(args.save)
    save.profile.win_all_tournaments()
    sign_or_keep(save, signing_key, args.unsigned)
    warn_all(rewrite_save(args.save, save))
    print('%s: every tournament won, %d%% complete (%s)' % (save_file_path(args.save), save.profile.percent_complete(),
                                                            'signed' if signing_key.available else 'not signed'))
    return 0


def run_to_json(args, signing_key):
    save = load_save(args.save)
    if signing_key.check(save) is SignatureStatus.INVALID:
        warn('the save\'s signature is invalid (converted anyway)')
    Path(args.out).write_text(viewer_json_text(to_viewer_json(save)), encoding='utf-8')
    print('%s: written' % args.out)
    return 0


def write_new_folder(save, args, signing_key):
    sign_or_keep(save, signing_key, args.unsigned)
    folder = output_folder(save, args.out, args.exact_folder)
    warn_all(write_save_folder(save, folder))
    print('%s: written (%s)' % (folder, 'signed' if signing_key.available else 'not signed'))
    return 0


def run_from_json(args, signing_key):
    return write_new_folder(from_viewer_json(load_viewer_json(args.profile)), args, signing_key)


def run_new(args, signing_key):
    return write_new_folder(new_save(args.name), args, signing_key)


def add_output_options(parser):
    parser.add_argument('out', help='the parent folder (the save folder is named by the game\'s hash of the name)')
    parser.add_argument('--exact-folder', action='store_true', help='write into OUT itself')
    parser.add_argument('--unsigned', action='store_true', help='write without a signing key (the game rejects it)')


def build_parser():
    parser = argparse.ArgumentParser(description='Quantum Redshift profile saves (savegame.sav + SaveMeta.xbx).')
    parser.add_argument('--signature-key', metavar='FILE',
                        help='the game\'s signing key (XboxSignatureKey, tools\\saves\\signature_key.py) as 32 hex digits')
    parser.add_argument('--cert-key', metavar='FILE', help='the XboxCERTKey as 32 hex digits (with --xbe: the same key)')
    parser.add_argument('--keys', metavar='FILE', default=str(KEYS_JSON),
                        help='keys.json (XboxSignatureKey / XboxCERTKey / XbeSignature), used without a key file '
                             '(default: keys.json in the repo)')
    parser.add_argument('--xbe', metavar='FILE', help='the game\'s default.xbe (default: game_data\\default.xbe)')
    commands = parser.add_subparsers(dest='command', required=True)
    for name, handler, text in (('info', run_info, 'show a profile'), ('verify', run_verify, 'check the signature'),
                                ('resign', run_resign, 'sign savegame.sav again, in place')):
        command = commands.add_parser(name, help=text)
        command.add_argument('save', help='a save folder or its savegame.sav')
        command.set_defaults(handler=handler)
    win_all = commands.add_parser('win-all', help='every tournament won (trophies and their unlocks), in place')
    win_all.add_argument('save', help='a save folder or its savegame.sav')
    win_all.add_argument('--unsigned', action='store_true', help='write without a signing key (the game rejects it)')
    win_all.set_defaults(handler=run_win_all)
    to_json = commands.add_parser('to-json', help='convert to the viewer\'s profile JSON')
    to_json.add_argument('save', help='a save folder or its savegame.sav')
    to_json.add_argument('out', help='the .json file to write')
    to_json.set_defaults(handler=run_to_json)
    from_json = commands.add_parser('from-json', help='convert a viewer profile JSON to a save folder')
    from_json.add_argument('profile', help='the viewer\'s profile .json')
    add_output_options(from_json)
    from_json.set_defaults(handler=run_from_json)
    new = commands.add_parser('new', help='a fresh profile, as the game creates it')
    new.add_argument('name', help='the profile name (1..9 characters)')
    add_output_options(new)
    new.set_defaults(handler=run_new)
    return parser


def main(argv=None):
    args = build_parser().parse_args(argv)
    try:
        signing_key = load_signing_key(KeyFiles(args.signature_key, args.cert_key, args.xbe, args.keys))
        if args.command == 'resign' and not signing_key.available:
            raise SigningKeyMissing('resign needs a key (--signature-key, --cert-key or keys.json)')
        return args.handler(args, signing_key)
    except (OSError, ValueError, SigningKeyMissing) as error:
        print('error: %s' % error, file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
