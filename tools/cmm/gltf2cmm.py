"""Write a Quantum Redshift model.cmm back from a .glb edited in Blender (the way back of cmm2gltf.py).

usage: python -I gltf2cmm.py <original model.cmm> <edited.glb> [out.cmm]    (default: <original>.new.cmm)

Only the geometry changes. Every mesh in the .glb, matched by its "qr_mesh" extra (Blender keeps it when the glTF
export has Include > Custom Properties ticked) or else by its name "meshNN", gets its vertices and triangles written
in its original vertex type, as a triangle list. Everything else is kept byte for byte: the header, the materials,
the bones, the surface blocks, the effects and their programs, the animation, the texture table and every mesh the
.glb leaves out. The game walks the file in a fixed order and ignores its stale pointers (docs\\models\\README.md "Data order
after the header"), so a mesh's vertex and index blocks may change size.

Not written: skinned meshes (cmm2gltf.py does not export them) and morph meshes (only explodefirelarge, never loaded).
The node transforms are not read: edit the vertices (Edit Mode), not the objects' placement. Vertex types 4..7 store
no normal; the rest keep the .glb's. A COLOR_0 attribute gives the D3DCOLOR; without one the vertices take the
mesh's first original colour (cmm2gltf.py leaves out colours that are all 0xfefefefe or 0xffffffff).
"""
import json
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cmm import CMM, MF_MORPH, MF_STRIP, MF_WEIGHTED, VERTEX_STRIDE

MESH_RECORD_SIZE = 200
MESH_VERTEX_COUNT, MESH_FLAGS, MESH_FACE_COUNT = 0x08, 0x0c, 0x28
MAX_VERTICES = 0xffff                      # the index data is u16
GLB_MAGIC, GLB_JSON, GLB_BIN = 0x46546C67, 0x4E4F534A, 0x004E4942
COMPONENT_FORMATS = {5120: 'b', 5121: 'B', 5122: 'h', 5123: 'H', 5125: 'I', 5126: 'f'}
COMPONENT_COUNTS = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}
NORMALIZED_RANGE = {5120: 127.0, 5121: 255.0, 5122: 32767.0, 5123: 65535.0}
DEFAULT_COLOUR = 0xfefefefe


class Geometry:
    """One mesh's vertices (position, normal, uv sets, D3DCOLOR or None) and triangles in the game's winding."""

    def __init__(self, vertices, triangles):
        self.vertices = vertices
        self.triangles = triangles


class Glb:
    def __init__(self, path):
        data = open(path, 'rb').read()
        magic, version, _ = struct.unpack_from('<3I', data, 0)
        if magic != GLB_MAGIC or version != 2:
            raise ValueError('%s: not a glTF 2.0 binary (.glb)' % path)
        self.doc, self.bin, at = None, b'', 12
        while at < len(data):
            length, kind = struct.unpack_from('<2I', data, at)
            chunk = data[at + 8:at + 8 + length]
            if kind == GLB_JSON:
                self.doc = json.loads(chunk)
            elif kind == GLB_BIN:
                self.bin = chunk
            at += 8 + length
        if self.doc is None:
            raise ValueError('%s: no JSON chunk' % path)

    def values(self, accessor_index):
        """An accessor's elements as tuples (normalised integers as 0..1 floats)."""
        a = self.doc['accessors'][accessor_index]
        view = self.doc['bufferViews'][a['bufferView']]
        fmt = COMPONENT_FORMATS[a['componentType']]
        components = COMPONENT_COUNTS[a['type']]
        size = struct.calcsize('<' + fmt)
        stride = view.get('byteStride') or size * components
        start = view.get('byteOffset', 0) + a.get('byteOffset', 0)
        scale = NORMALIZED_RANGE.get(a['componentType']) if a.get('normalized') else None
        out = []
        for i in range(a['count']):
            element = struct.unpack_from('<%d%s' % (components, fmt), self.bin, start + i * stride)
            out.append(tuple(x / scale for x in element) if scale else element)
        return out

    def mesh_geometries(self):
        """{game mesh index: Geometry} for every glTF mesh that names one."""
        found = {}
        for mesh in self.doc.get('meshes', []):
            index = game_mesh_index(mesh)
            if index is None:
                continue
            if index in found:
                raise ValueError('two meshes of the .glb are game mesh %d' % index)
            found[index] = self.geometry(mesh)
        return found

    def geometry(self, mesh):
        vertices, triangles = [], []
        for primitive in mesh['primitives']:
            if primitive.get('mode', 4) != 4:
                raise ValueError('mesh %s: only triangles can be written' % mesh.get('name'))
            first = len(vertices)
            vertices += self.primitive_vertices(primitive['attributes'])
            indices = ([i for (i,) in self.values(primitive['indices'])] if 'indices' in primitive
                       else list(range(len(vertices) - first)))
            for k in range(0, len(indices) - 2, 3):
                a, b, c = indices[k:k + 3]
                triangles.append((first + a, first + c, first + b))   # glTF counter-clockwise -> the game's clockwise
        return Geometry(vertices, triangles)

    def primitive_vertices(self, attributes):
        positions = self.values(attributes['POSITION'])
        count = len(positions)
        normals = self.values(attributes['NORMAL']) if 'NORMAL' in attributes else [(0.0, 1.0, 0.0)] * count
        uv_sets, k = [], 0
        while 'TEXCOORD_%d' % k in attributes:
            uv_sets.append(self.values(attributes['TEXCOORD_%d' % k]))
            k += 1
        colours = [d3d_colour(c) for c in self.values(attributes['COLOR_0'])] if 'COLOR_0' in attributes else [None] * count
        return [(positions[i], normals[i], [uvs[i] for uvs in uv_sets], colours[i]) for i in range(count)]


def game_mesh_index(mesh):
    extras = mesh.get('extras') or {}
    if 'qr_mesh' in extras:
        return int(extras['qr_mesh'])
    match = re.match(r'mesh(\d+)', mesh.get('name', ''))
    return int(match.group(1)) if match else None


def d3d_colour(rgba):
    """A COLOR_0 element (0..1 floats, 3 or 4 of them) as a D3DCOLOR (A8R8G8B8)."""
    r, g, b = (round(max(0.0, min(1.0, x)) * 255) for x in rgba[:3])
    a = round(max(0.0, min(1.0, rgba[3])) * 255) if len(rgba) > 3 else 255
    return (a << 24) | (r << 16) | (g << 8) | b


def first_colour(cmm, mesh):
    colours = cmm.colours(mesh) if mesh.num_verts else []
    if not colours:
        return DEFAULT_COLOUR
    r, g, b, a = colours[0]
    return (a << 24) | (r << 16) | (g << 8) | b


def vertex_bytes(vtype, geometry, fallback_colour):
    """The vertices in the game's layout: vtype 0..3 position, normal, D3DCOLOR, uv sets; 4..7 without the normal."""
    uv_count = (vtype & 3) + 1
    out = bytearray()
    for position, normal, uvs, colour in geometry.vertices:
        out += struct.pack('<3f', *position)
        if vtype < 4:
            out += struct.pack('<3f', *normal)
        out += struct.pack('<I', fallback_colour if colour is None else colour)
        for k in range(uv_count):
            out += struct.pack('<2f', *(uvs[k] if k < len(uvs) else (0.0, 0.0)))
    if len(out) != len(geometry.vertices) * VERTEX_STRIDE[vtype]:
        raise AssertionError('vertex type %d: wrote %d bytes' % (vtype, len(out)))
    return bytes(out)


def index_bytes(triangles):
    return struct.pack('<%dH' % (len(triangles) * 3), *[i for t in triangles for i in t])


def check_writable(mesh, geometry):
    if mesh.flags & (MF_WEIGHTED | MF_MORPH):
        raise ValueError('mesh %d: skinned and morph meshes cannot be written' % mesh.index)
    if len(geometry.vertices) > MAX_VERTICES:
        raise ValueError('mesh %d: %d vertices, the game indexes at most %d'
                         % (mesh.index, len(geometry.vertices), MAX_VERTICES))
    if any(i >= len(geometry.vertices) for t in geometry.triangles for i in t):
        raise ValueError('mesh %d: a triangle refers past the vertices' % mesh.index)


def rebuild(cmm, geometries):
    """The model with the given meshes' geometry replaced ({mesh index: Geometry}); bytes."""
    d = cmm.data
    for index, geometry in geometries.items():
        if not 0 <= index < len(cmm.meshes):
            raise ValueError('mesh %d: the model has %d meshes' % (index, len(cmm.meshes)))
        check_writable(cmm.meshes[index], geometry)
    head = bytearray(d[:cmm.vertex_data_ofs])
    vertex_blocks, index_blocks = bytearray(), bytearray()
    for m in cmm.meshes:
        geometry = geometries.get(m.index)
        if geometry is None:
            vertex_blocks += d[m.vertex_block[0]:m.vertex_block[1]]
            index_blocks += d[m.index_block[0]:m.index_block[1]]
            continue
        vertex_blocks += vertex_bytes(m.vtype, geometry, first_colour(cmm, m))
        index_blocks += index_bytes(geometry.triangles)
        struct.pack_into('<I', head, m.ofs + MESH_VERTEX_COUNT, len(geometry.vertices))
        struct.pack_into('<I', head, m.ofs + MESH_FACE_COUNT, len(geometry.triangles))
        struct.pack_into('<I', head, m.ofs + MESH_FLAGS, m.flags & ~MF_STRIP)
    return bytes(head + vertex_blocks + index_blocks + d[cmm.index_data_end:])


def write(original, glb_path, out):
    cmm = CMM(original)
    geometries = Glb(glb_path).mesh_geometries()
    with open(out, 'wb') as f:
        f.write(rebuild(cmm, geometries))
    print('%s + %s -> %s  (%d of %d meshes written)' % (original, glb_path, out, len(geometries), len(cmm.meshes)))


if __name__ == '__main__':
    args = sys.argv[1:]
    if len(args) < 2:
        print(__doc__)
        sys.exit(1)
    write(args[0], args[1], args[2] if len(args) > 2 else os.path.splitext(args[0])[0] + '.new.cmm')
