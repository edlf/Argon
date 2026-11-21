"""Convert a Quantum Redshift model.cmm (+ its tex*.dds) to a .glb for Blender.

usage: python -I cmm2gltf.py <model folder or model.cmm> [out.glb] [--lotex]

- Every bone becomes a glTF node (same names, same hierarchy, local matrices),
  meshes hang off the bone that owns them. Empty bones are attachment points.
- Coordinates: the game's data is effectively right-handed Y-up (it renders with a
  mirrored projection), same as glTF, so positions/normals/matrices are copied as-is.
  The game's front faces are clockwise, so triangle order is reversed for glTF (CCW).
  (Before 2026-11-10 this exporter mirrored Z, which mirrored every model.)
- Material base colour = first texture stage. The full texture list and raw
  mesh/bone ids are stored in node/mesh "extras" for a future re-import.
"""
import json, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cmm import CMM
import xdds

def gltf_matrix(m):
    """D3D row-vector matrix -> glTF column-major list."""
    # row-vector M == column-vector M^T; glTF stores column-major, which is M's rows in order.
    return [m[r][c] for r in range(4) for c in range(4)]


class GLB:
    def __init__(self):
        self.bin = bytearray()
        self.views, self.accessors = [], []

    def _view(self, data, target=None):
        while len(self.bin) % 4:
            self.bin += b'\0'
        v = {'buffer': 0, 'byteOffset': len(self.bin), 'byteLength': len(data)}
        if target:
            v['target'] = target
        self.bin += data
        self.views.append(v)
        return len(self.views) - 1

    def accessor(self, values, ncomp, ctype='f', target=34962, minmax=False):
        flat = [x for v in values for x in (v if ncomp > 1 else (v,))]
        fmt = {'f': 'f', 'H': 'H', 'I': 'I', 'B': 'B'}[ctype]
        data = struct.pack('<%d%s' % (len(flat), fmt), *flat)
        a = {'bufferView': self._view(data, target),
             'componentType': {'f': 5126, 'H': 5123, 'I': 5125, 'B': 5121}[ctype],
             'count': len(values),
             'type': {1: 'SCALAR', 2: 'VEC2', 3: 'VEC3', 4: 'VEC4'}[ncomp]}
        if ctype == 'B':
            a['normalized'] = True
        if minmax:
            a['min'] = [min(v[k] for v in values) for k in range(ncomp)]
            a['max'] = [max(v[k] for v in values) for k in range(ncomp)]
        self.accessors.append(a)
        return len(self.accessors) - 1

    def image(self, png):
        return self._view(png)


def convert(src, out=None, lotex=False):
    folder = src if os.path.isdir(src) else os.path.dirname(src)
    path = os.path.join(folder, 'model.cmm') if os.path.isdir(src) else src
    name = os.path.basename(os.path.normpath(folder))
    out = out or os.path.join(folder, name + '.glb')
    c = CMM(path)
    g = GLB()
    doc = {'asset': {'version': '2.0', 'generator': 'QR cmm2gltf'},
           'scene': 0, 'scenes': [{'name': name, 'nodes': []}],
           'nodes': [], 'meshes': [], 'materials': [], 'textures': [], 'images': [],
           'samplers': [{'magFilter': 9729, 'minFilter': 9987, 'wrapS': 10497, 'wrapT': 10497}]}

    tex_cache = {}
    def texture(i):
        if i in tex_cache:
            return tex_cache[i]
        p = os.path.join(folder, ('lotex%d.dds' if lotex else 'tex%d.dds') % i)
        tex_cache[i] = None
        if os.path.exists(p):
            try:
                w, h, px = xdds.decode(p)
            except ValueError as e:
                print('  skip texture:', e)
                return None
            doc['images'].append({'name': os.path.basename(p)[:-4], 'mimeType': 'image/png',
                                  'bufferView': g.image(xdds.png_bytes(w, h, px))})
            doc['textures'].append({'source': len(doc['images']) - 1, 'sampler': 0})
            tex_cache[i] = len(doc['textures']) - 1
        return tex_cache[i]

    mat_cache = {}
    def material(texlist):
        key = tuple(texlist)
        if key in mat_cache:
            return mat_cache[key]
        mat = {'name': 'tex_' + '_'.join(map(str, key)) if key else 'untextured',
               'pbrMetallicRoughness': {'metallicFactor': 0.0, 'roughnessFactor': 0.6},
               'doubleSided': False, 'extras': {'qr_textures': list(key)}}
        if key:
            t = texture(key[0])
            if t is not None:
                mat['pbrMetallicRoughness']['baseColorTexture'] = {'index': t}
        else:
            mat['pbrMetallicRoughness']['baseColorFactor'] = [0.6, 0.6, 0.6, 1.0]
        doc['materials'].append(mat)
        mat_cache[key] = len(doc['materials']) - 1
        return mat_cache[key]

    def mesh_gltf(m):
        if m.vert_ofs is None or m.flags & 0x40000:
            print('  mesh %d: skinned mesh not supported yet, skipped' % m.index)
            return None
        verts = c.vertices(m)
        pos = [tuple(p) for p, n, u in verts]
        nrm = [tuple(n) for p, n, u in verts]
        attrs = {'POSITION': g.accessor(pos, 3, minmax=True),
                 'NORMAL': g.accessor(nrm, 3)}
        for k in range(len(verts[0][2]) if verts else 0):
            attrs['TEXCOORD_%d' % k] = g.accessor([u[k] for p, n, u in verts], 2)
        cols = c.colours(m)
        if cols and any(col != (254, 254, 254, 254) and col != (255, 255, 255, 255) for col in cols):
            attrs['COLOR_0'] = g.accessor(cols, 4, 'B')   # prelit colour + alpha (D3DCOLOR)
        tris = c.triangles(m)
        idx = g.accessor([i for a, b, c3 in tris for i in (a, c3, b)], 1, 'H', 34963)   # CW -> CCW
        doc['meshes'].append({
            'name': 'mesh%02d' % m.index,
            'primitives': [{'attributes': attrs, 'indices': idx, 'material': material(m.tex)}],
            'extras': {'qr_mesh': m.index, 'qr_flags': m.flags, 'qr_vtype': m.vtype,
                       'qr_textures': m.tex, 'qr_collidable': m.collidable,
                       'qr_surface_flags': m.surface_flags}})
        return len(doc['meshes']) - 1

    for b in c.bones:
        node = {'name': b.name or 'bone%02d' % b.index, 'matrix': gltf_matrix(b.local),
                'extras': {'qr_bone': b.index}}
        doc['nodes'].append(node)
    for b in c.bones:
        if 0 <= b.parent < len(c.bones):
            doc['nodes'][b.parent].setdefault('children', []).append(b.index)
        else:
            doc['scenes'][0]['nodes'].append(b.index)
    # meshes: one on the bone node itself, extras (e.g. 2 meshes on one bone) as
    # child nodes with identity transform so each stays a separate Blender object
    for b in c.bones:
        for k, m in enumerate(b.meshes):
            mi = mesh_gltf(m)
            if mi is None:
                continue
            if k == 0:
                doc['nodes'][b.index]['mesh'] = mi
            else:
                doc['nodes'].append({'name': '%s.mesh%d' % (doc['nodes'][b.index]['name'], k),
                                     'mesh': mi, 'extras': {'qr_bone': b.index}})
                doc['nodes'][b.index].setdefault('children', []).append(len(doc['nodes']) - 1)
    # bone-less meshes = static world geometry (tracks), vertices in world space
    groups = {}
    for m in c.meshes:
        if m.bone is not None:
            continue
        mi = mesh_gltf(m)
        if mi is None:
            continue
        if m.collidable:
            # untextured collidable meshes = invisible walls / kill planes
            gname = 'world_collision' if m.tex else 'world_invisible_walls'
        else:
            gname = 'world_visual'
        if gname not in groups:
            doc['nodes'].append({'name': gname, 'children': []})
            groups[gname] = len(doc['nodes']) - 1
            doc['scenes'][0]['nodes'].append(groups[gname])
        doc['nodes'].append({'name': 'mesh%03d' % m.index, 'mesh': mi,
                             'extras': {'qr_mesh': m.index}})
        doc['nodes'][groups[gname]]['children'].append(len(doc['nodes']) - 1)
    # identity matrices are noise in Blender's UI
    for n in doc['nodes']:
        if n.get('matrix') == [1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0, 0, 0, 0, 0, 1.0]:
            del n['matrix']

    doc['bufferViews'] = g.views
    doc['accessors'] = g.accessors
    doc['buffers'] = [{'byteLength': len(g.bin)}]
    for k in ('textures', 'images'):
        if not doc[k]:
            del doc[k]
    js = json.dumps(doc, separators=(',', ':')).encode()
    js += b' ' * (-len(js) % 4)
    bn = bytes(g.bin) + b'\0' * (-len(g.bin) % 4)
    with open(out, 'wb') as f:
        f.write(struct.pack('<3I', 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(bn)))
        f.write(struct.pack('<2I', len(js), 0x4E4F534A) + js)
        f.write(struct.pack('<2I', len(bn), 0x004E4942) + bn)
    print('%s -> %s  (%d meshes, %d bones, %d textures)'
          % (path, out, len(doc['meshes']), len(c.bones), len(doc.get('textures', []))))
    return out


if __name__ == '__main__':
    a = [x for x in sys.argv[1:] if not x.startswith('--')]
    if not a:
        print(__doc__); sys.exit(1)
    convert(a[0], a[1] if len(a) > 1 else None, lotex='--lotex' in sys.argv)
