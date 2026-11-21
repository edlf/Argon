"""Survey the .cmm data the effect setups and the texture loader read (docs\\models\\effects.md "Effects", docs\\models\\materials.md "Texture table").

For every effect record name: how many meshes use it (material +0xe0), in which files, with which vertex types,
stage counts, mesh flags, surface setups, blend descriptors and surface flags. Then the texture table's entries
(type, frame count and the two dwords the loader skips), the materials' unread stage words (+0x2c / +0x30) and the
animated textures that meshes share on one stage at different rates (the bind cache, docs\\models\\materials.md "Texture table").

usage: python -I effect_survey.py [game_data]      (default: ..\\..\\game_data)
"""
import glob
import os
import struct
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cmm  # noqa: E402

EFFECT_SIZE = 0x5bc
MATERIAL_SIZE = 0xec
STAGE_SIZE = 0x34
FIRST_STAGE = 0x0c


def u32(d, o):
    return struct.unpack_from('<I', d, o)[0]


def effect_names(c):
    d, start = c.data, c.end_of_mesh_data
    count = c.num_0x30 if c.num_0x30 < 64 else 0
    return [d[start + i * EFFECT_SIZE:start + i * EFFECT_SIZE + 0x40].split(b'\0')[0].decode('latin-1')
            for i in range(count)]


def mesh_effect(c, m, names):
    if m.node < 0 or m.node >= c.num_nodes:
        return None
    index = struct.unpack_from('<i', c.data, c.nodes_ofs + m.node * MATERIAL_SIZE + 0xe0)[0]
    return names[index] if 0 <= index < len(names) else None


def surface_of(c, m):
    if m.pushbuf_ofs is None:
        return 'none'
    d, o = c.data, m.pushbuf_ofs
    return 'setup %d blend %#x flags %#x' % (u32(d, o + 4), u32(d, o + 8), u32(d, o + 12))


def survey_effects(models, out):
    uses = defaultdict(Counter)
    files = defaultdict(Counter)
    for src, c in models:
        names = effect_names(c)
        for m in c.meshes:
            name = mesh_effect(c, m, names)
            if name is None:
                continue
            uses[name]['vtype %d stages %d mesh flags %#x %s' % (m.vtype, m.num_tex, m.flags, surface_of(c, m))] += 1
            files[name][src] += 1
    for name in sorted(uses, key=lambda n: -sum(files[n].values())):
        out.write('\n== %s: %d meshes in %d files\n' % (name, sum(files[name].values()), len(files[name])))
        for kind, n in uses[name].most_common(8):
            out.write('  %5d  %s\n' % (n, kind))
        out.write('  files: %s\n' % ', '.join('%s x%d' % f for f in files[name].most_common(12)))


def survey_texture_tables(models, out):
    kinds = Counter()
    for src, c in models:
        if c.textable_ofs is None:
            continue
        for i in range(c.num_textures):
            o = c.textable_ofs + i * 16
            kind, frames = u32(c.data, o), u32(c.data, o + 4)
            kinds[(kind, frames if frames > 1 else 1)] += 1
    out.write('\n== texture table: (type, frames) -> entries\n')
    for (kind, frames), n in sorted(kinds.items()):
        out.write('  type %d frames %3d: %d\n' % (kind, frames, n))


def survey_unread_stage_words(models, out):
    out.write('\n== material stage words +0x2c / +0x30 (not read by 0xba660) that are non-zero\n')
    for src, c in models:
        for i in range(c.num_nodes):
            for s in range(4):
                o = c.nodes_ofs + i * MATERIAL_SIZE + FIRST_STAGE + s * STAGE_SIZE + 0x2c
                a, b = struct.unpack_from('<2f', c.data, o)
                if a or b:
                    out.write('  %s material %d stage %d: %g %g\n' % (src, i, s, a, b))


def stage_rate(c, m, stage):
    if m.node < 0 or m.node >= c.num_nodes:
        return 60.0   # the slot's rate from the load (0xb67a0)
    return struct.unpack_from('<f', c.data, c.nodes_ofs + m.node * MATERIAL_SIZE + FIRST_STAGE + stage * STAGE_SIZE + 0x18)[0]


def texture_frames(c, texture):
    if c.textable_ofs is None or not 0 <= texture < c.num_textures:
        return 1
    return max(1, u32(c.data, c.textable_ofs + texture * 16 + 4))


def survey_shared_animated_slots(models, out):
    """Meshes binding one animated texture on one stage with different rates: drawn in a row, they all show the
    first one's frame (d3d_set_texture's bind cache, 0xcbbd2)."""
    out.write('\n== animated textures shared on one stage with different rates (mesh: rate)\n')
    for src, c in models:
        users = defaultdict(list)
        for m in c.meshes:
            for stage, texture in enumerate(m.tex[:4]):
                if texture_frames(c, texture) > 1:
                    users[(stage, texture)].append((m.index, stage_rate(c, m, stage)))
        for (stage, texture), meshes in sorted(users.items()):
            if len(set(rate for _, rate in meshes)) > 1:
                out.write('  %s stage %d texture %d (%d frames): %s\n' % (
                    src, stage, texture, texture_frames(c, texture), ', '.join('%d: %g' % m for m in meshes)))


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    game = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, '..', '..', 'game_data')
    models = []
    for p in sorted(glob.glob(os.path.join(game, 'data', '**', '*.cmm'), recursive=True)):
        try:
            models.append((os.path.relpath(p, game), cmm.CMM(p)))
        except (ValueError, struct.error, KeyError) as e:
            print('skip', p, e, file=sys.stderr)
    survey_effects(models, sys.stdout)
    survey_texture_tables(models, sys.stdout)
    survey_unread_stage_words(models, sys.stdout)
    survey_shared_animated_slots(models, sys.stdout)


if __name__ == '__main__':
    main()
