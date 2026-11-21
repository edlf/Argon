"""Export every ship (racing craft + front-end menu ships) to tools/cmm/export/.
usage: python -I export_all.py [--lotex]
"""
import glob, os, sys

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, here)
from cmm2gltf import convert

game = os.path.normpath(os.path.join(here, '..', '..', 'game_data'))
lotex = '--lotex' in sys.argv
only = [a for a in sys.argv[1:] if not a.startswith('--')]   # e.g. "tracks"
jobs = (('craft', 'data/craft/*/model.cmm'),
        ('feship', 'data/characters/feship_*/model.cmm'),
        ('tracks', 'data/tracks/*/model.cmm'),
        ('tracks_sky', 'data/tracks/*/sky/model.cmm'))
for sub, pattern in jobs:
    if only and sub not in only:
        continue
    os.makedirs(os.path.join(here, 'export', sub), exist_ok=True)
    for p in sorted(glob.glob(os.path.join(game, pattern))):
        name = os.path.basename(os.path.dirname(p))
        if sub == 'tracks_sky':
            name = os.path.basename(os.path.dirname(os.path.dirname(p)))
        convert(p, os.path.join(here, 'export', sub, name + '.glb'), lotex=lotex)
