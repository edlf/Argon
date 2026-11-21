"""Read a Quantum Redshift replay (event buffer) and print its header, first events and totals.

usage: python -I tools/saves/replay.py [replays dir = game_data/replays] [n = 1] [events to print = 12]
Format: docs/replays/README.md. Reads replay<n>.info / .index / .events.
"""
import collections
import os
import struct
import sys

CRAFT_EVENT_SIZE = 0x58
CHUNK_HEADER_SIZE = 0x10
INDEX_END_TIME = -1.0
ORIENTATION_SCALE = 1.0 / 4096
WEAPON_SIGNALS = {1: 'craft weapon 0x6ac90', 2: 'launch', 3: 'projectile done', 4: 'impact', 5: 'none', 6: 'energy +0x9c',
                  7: 'fire all', 8: 'pickup taken', 9: 'pickup respawn', 0xb: '0x6a6c0', 0xc: '0x64b30 + impact'}
CONTROL_BITS = ((1, 'throttle'), (2, 'fire'), (4, 'left'), (8, 'right'), (0x10, 'dive'), (0x20, 'climb'),
                (0x40, 'brake'), (0x80, 'airbrake L'), (0x100, 'airbrake R'))


def name_hash(name):
    """obj_send_message 0xb4e00: the id an event carries for the object it is addressed to."""
    raw = name.encode('ascii')
    whole = len(raw) & ~3
    h = 0
    for i in range(0, whole, 4):
        h ^= (struct.unpack_from('<I', raw, i)[0] + i) & 0xffffffff
    for k in range(len(raw) & 3):
        c = raw[len(raw) - 1 - k]
        h ^= ((c - 256 if c > 127 else c) + k) & 0xffffffff
    return h


NAMES = {name_hash('craft%d' % i): 'craft%d' % i for i in range(16)}
NAMES[name_hash('WeaponEvents')] = 'WeaponEvents'


def read_info(path):
    pairs = [line.split() for line in open(path, encoding='ascii') if len(line.split()) == 2]
    return dict(pairs)


def read_index(path):
    data = open(path, 'rb').read()
    entries = []
    for time, offset in struct.iter_unpack('<fI', data):
        if time == INDEX_END_TIME:
            break
        entries.append((time, offset))
    return entries


def decode_craft_event(e):
    pos = struct.unpack_from('<3f', e, 0x18)
    vel = struct.unpack_from('<3f', e, 0x24)
    spin = struct.unpack_from('<3f', e, 0x30)
    axes = [struct.unpack_from('<h', e, 0x3c + 4 * i)[0] * ORIENTATION_SCALE for i in range(6)]
    bits = struct.unpack_from('<I', e, 0x54)[0]
    return pos, vel, spin, axes, bits


def describe(e, time):
    kind, size, target = struct.unpack_from('<3I', e, 4)
    name = NAMES.get(target, '%08x' % target)
    if kind == 1:
        pos, vel, spin, axes, bits = decode_craft_event(e)
        held = ','.join(n for b, n in CONTROL_BITS if bits & b) or '-'
        return ('%8.3f %-6s pos (%.2f %.2f %.2f) vel (%.2f %.2f %.2f) w (%.3f %.3f %.3f) x (%.3f %.3f %.3f) y (%.3f %.3f %.3f) %s'
                % ((time, name) + pos + vel + spin + tuple(axes) + (held,)))
    wkind = struct.unpack_from('<I', e, 0x18)[0]
    slot, weapon, craft, extra = e[0x1c], e[0x1d], e[0x1e], e[0x1f]
    seq = struct.unpack_from('<I', e, 0x20)[0]
    if wkind == 2:
        signal = struct.unpack_from('<I', e, 0x24)[0]
        what = 'signal %d %s' % (signal, WEAPON_SIGNALS.get(signal, '?'))
    else:
        what = 'projectile pos (%.2f %.2f %.2f) vel (%.2f %.2f %.2f)' % struct.unpack_from('<6f', e, 0x24)
    return '%8.3f %s #%d craft %d weapon %d slot %d (%d) %s' % (time, name, seq, craft, weapon, slot, extra, what)


def walk_chunks(events, index):
    """Yields (index time, chunk header, [entries]) for each index entry."""
    for time, offset in index:
        size, chunk_time, continuous, critical = struct.unpack_from('<IfII', events, offset)
        entries, at = [], offset + CHUNK_HEADER_SIZE
        for _ in range(continuous + critical):
            entry_size = struct.unpack_from('<I', events, at + 8)[0]
            entries.append(events[at:at + entry_size])
            at += entry_size
        assert at == offset + size, 'chunk at %#x: entries end at %#x, size says %#x' % (offset, at, offset + size)
        yield time, (size, chunk_time, continuous, critical), entries


def main():
    folder = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), '..', '..', 'game_data', 'replays')
    n = sys.argv[2] if len(sys.argv) > 2 else '1'
    shown = int(sys.argv[3]) if len(sys.argv) > 3 else 12
    stem = os.path.join(folder, 'replay' + n)
    print('info', read_info(stem + '.info'))
    index = read_index(stem + '.index')
    events = open(stem + '.events', 'rb').read()
    print('index: %d entries, %.3f .. %.3f s; events %d bytes, used %d' % (len(index), index[0][0], index[-1][0], len(events),
                                                                         index[-1][1] + struct.unpack_from('<I', events, index[-1][1])[0]))
    counts, signals, gaps = collections.Counter(), collections.Counter(), []
    last_time = None
    printed = 0
    for time, header, entries in walk_chunks(events, index):
        if last_time is not None:
            gaps.append(time - last_time)
        last_time = time
        for e in entries:
            kind, size, target = struct.unpack_from('<3I', e, 4)
            counts[(kind, size, NAMES.get(target, '%08x' % target))] += 1
            if kind == 2 and struct.unpack_from('<I', e, 0x18)[0] == 2:
                signals[struct.unpack_from('<I', e, 0x24)[0]] += 1
            if printed < shown:
                print('  chunk t %.3f (header %s)  ' % (time, header) + describe(e, struct.unpack_from('<f', e, 0x14)[0]))
                printed += 1
    print('frame step: min %.4f max %.4f mean %.4f s' % (min(gaps), max(gaps), sum(gaps) / len(gaps)))
    for (kind, size, name), c in sorted(counts.items()):
        print('  type %d size %#x %-12s %d' % (kind, size, name, c))
    print('  weapon signals', {'%d %s' % (k, WEAPON_SIGNALS.get(k, '?')): c for k, c in sorted(signals.items())})


if __name__ == '__main__':
    main()
