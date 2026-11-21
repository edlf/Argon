"""Reader for Quantum Redshift .cmm model files (data\\<name>\\model.cmm).

Layout follows the game's pointer-fixup routine cmm_fixup_pointers (XBE 0x000b6070):
the file is a raw dump of structs whose pointer fields are stale; data blocks
follow the header in a fixed order, so we rebuild offsets the same way.
"""
import struct

VERTEX_STRIDE = {0: 0x24, 1: 0x2c, 2: 0x34, 3: 0x3c,   # float normal, 1..4 UV sets
                 4: 0x18, 5: 0x20, 6: 0x28, 7: 0x30}   # packed normal, 1..4 UV sets
MORPH_STRIDE = {7: 0x1c, 3: 0x18, 5: 0x10, 1: 0x0c, 4: 0x04}

# mesh record (200 bytes) flags
MF_STRIP = 0x1        # indices are one triangle strip of (faces + 2) u16
MF_MORPH = 0x2        # has morph/extra vertex streams
MF_PUSHBUF = 0x20     # has a size-prefixed precompiled pushbuffer block
MF_WEIGHTED = 0x40000 # skinned vertices in a vertex-buffer resource


class Mesh:
    pass


class CMM:
    def __init__(self, path):
        with open(path, 'rb') as f:
            self.data = d = f.read()
        self.path = path
        u = lambda o: struct.unpack_from('<I', d, o)[0]
        self.version = u(0x00)
        self.num_meshes = u(0x04)
        self.num_textures = u(0x08)
        self.flags = u(0x0c)
        self.num_nodes = u(0x18)
        self.num_bones = u(0x20)
        self.num_0x30 = u(0x30)
        self.num_0x40 = u(0x40)
        if self.version != 300:
            raise ValueError('unknown cmm version %d' % self.version)

        cur = 0x48
        self.meshes = []
        for i in range(self.num_meshes):
            m = Mesh()
            m.index = i
            m.ofs = cur + i * 200
            m.raw = d[m.ofs:m.ofs + 200]
            m.num_verts = u(m.ofs + 0x08)
            m.flags = u(m.ofs + 0x0c)
            m.num_faces = u(m.ofs + 0x28)
            m.num_tex = u(m.ofs + 0x34)
            m.tex = list(struct.unpack_from('<%dh' % max(m.num_tex, 0), d, m.ofs + 0x38))[:m.num_tex]
            m.node = struct.unpack_from('<h', d, m.ofs + 0x48)[0]
            m.vtype = struct.unpack_from('<h', d, m.ofs + 0xc2)[0]
            self.meshes.append(m)
        cur += self.num_meshes * 200

        self.nodes_ofs = cur
        if self.num_nodes > 0:
            cur += self.num_nodes * 0xec
        self.bones_ofs = None
        if self.num_bones > 0:
            cur = (cur + 15) & ~15
            self.bones_ofs = cur
            cur += self.num_bones * 0x1a0

        for m in self.meshes:                     # surface blocks (flag 0x20)
            # 48-byte block; +0x0c bit 2 = collidable (collision.cpp builder
            # 0x000d29c0), bit 1 = ?, +0x14/+0x18 copied into the collision mesh.
            m.pushbuf_ofs = None
            m.surface_flags = 0
            if m.flags & MF_PUSHBUF:
                m.pushbuf_ofs = cur
                m.surface_flags = u(cur + 0x0c)
                m.surface_params = (u(cur + 0x14), u(cur + 0x18))
                cur += u(cur)
            m.collidable = bool(m.surface_flags & 2)

        self.vertex_data_ofs = cur
        for m in self.meshes:                     # vertex data
            m.vertex_block = (cur, None)          # this mesh's vertices, morph streams and skin block
            m.vert_ofs = m.morph_ofs = m.skin_ofs = None
            if not (m.flags & MF_WEIGHTED):
                m.vert_ofs = cur
                cur += m.num_verts * VERTEX_STRIDE[m.vtype]
            if m.flags & MF_MORPH:
                m.morph_ofs = cur
                kind, n = u(cur + 4), u(cur + 8)
                cur += 0x10 + n * 8
                m.morph_streams = []
                for _ in range(n):
                    m.morph_streams.append(cur)
                    cur += m.num_verts * MORPH_STRIDE.get(kind, 0)
            if m.flags & MF_WEIGHTED:
                m.skin_ofs = cur
                nb = u(cur + 4)
                cur += 0x38
                m.skin_bones = nb
                m.vert_ofs = cur
                m.skin_stride = 0x38 + (nb - 2) * 0x10
                cur += m.num_verts * m.skin_stride
            m.vertex_block = (m.vertex_block[0], cur)

        for m in self.meshes:                     # index data
            m.idx_ofs = cur
            if m.flags & MF_STRIP:
                cur += 4 + m.num_faces * 2
            else:
                cur += m.num_faces * 6
            m.index_block = (m.idx_ofs, cur)
        self.index_data_end = cur

        self._read_bones()
        self.textable_ofs = None
        if self.flags & 0x1000 and u(0x2c) and self.num_textures:
            self.textable_ofs = cur
            cur += self.num_textures * 0x10
        self.end_of_mesh_data = cur

    # ---- skeleton ----
    # bone record (0x1a0 bytes):
    #   +0x000 4x4 local matrix (D3D row-vector, translation in last row)
    #   +0x080 4x4 world matrix as saved (spinning parts may be mid-rotation)
    #   +0x11c scale xyz?, +0x128 index, +0x12c number of meshes on this bone
    #   +0x140 name (char[])
    #   +0x180 s16 parent, +0x182 first child, +0x184 prev sibling, +0x186 next sibling
    # mesh record +0x00 holds the owning bone index.
    def _read_bones(self):
        d = self.data
        self.bones = []
        for i in range(self.num_bones):
            o = self.bones_ofs + i * 0x1a0
            b = Mesh()
            b.index, b.ofs = i, o
            b.name = d[o + 0x140:o + 0x170].split(b'\0')[0].decode('latin-1')
            b.local = [list(struct.unpack_from('<4f', d, o + r * 16)) for r in range(4)]
            b.world = [list(struct.unpack_from('<4f', d, o + 0x80 + r * 16)) for r in range(4)]
            b.num_meshes = struct.unpack_from('<I', d, o + 0x12c)[0]
            b.parent, b.child, b.prev, b.next = struct.unpack_from('<4h', d, o + 0x180)
            b.meshes = []
            self.bones.append(b)
        for m in self.meshes:
            bi = struct.unpack_from('<i', d, m.ofs)[0]
            m.bone = self.bones[bi] if 0 <= bi < len(self.bones) else None
            if m.bone:
                m.bone.meshes.append(m)

    # ---- geometry ----
    def vertices(self, m):
        """List of (pos, normal, [uv...]) for a non-skinned mesh.
        vtype 0..3: pos, normal, D3DCOLOR, uvs. vtype 4..7: pos, D3DCOLOR, uvs; they store no normal,
        so it is built from the triangles (area-weighted, front faces clockwise)."""
        d, st, out = self.data, VERTEX_STRIDE[m.vtype], []
        packed = m.vtype >= 4
        nuv = (m.vtype & 3) + 1
        for i in range(m.num_verts):
            o = m.vert_ofs + i * st
            pos = struct.unpack_from('<3f', d, o)
            if packed:
                nrm = (0.0, 0.0, 0.0)
                o2 = o + 16        # +12 = diffuse colour (D3DCOLOR)
            else:
                nrm = struct.unpack_from('<3f', d, o + 12)
                o2 = o + 24 + 4    # +24 = diffuse colour (D3DCOLOR)
            uvs = [struct.unpack_from('<2f', d, o2 + k * 8) for k in range(nuv)]
            out.append((pos, nrm, uvs))
        if packed:
            acc = [[0.0, 0.0, 0.0] for _ in out]
            for a, b, c in self.triangles(m):
                if max(a, b, c) >= len(out):
                    continue
                pa, pb, pc = out[a][0], out[b][0], out[c][0]
                e1 = [pc[k] - pa[k] for k in range(3)]
                e2 = [pb[k] - pa[k] for k in range(3)]
                n = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
                for v in (a, b, c):
                    for k in range(3):
                        acc[v][k] += n[k]
            for i, (pos, _, uvs) in enumerate(out):
                l = (acc[i][0] ** 2 + acc[i][1] ** 2 + acc[i][2] ** 2) ** 0.5
                out[i] = (pos, tuple(x / l for x in acc[i]) if l > 1e-12 else (0.0, 1.0, 0.0), uvs)
        return out

    def colours(self, m):
        """Vertex colours (r, g, b, a) bytes, from the D3DCOLOR of every vertex type."""
        d, st = self.data, VERTEX_STRIDE[m.vtype]
        co = 12 if m.vtype >= 4 else 24
        out = []
        for i in range(m.num_verts):
            v = struct.unpack_from('<I', d, m.vert_ofs + i * st + co)[0]
            out.append(((v >> 16) & 255, (v >> 8) & 255, v & 255, v >> 24))
        return out

    def triangles(self, m):
        d = self.data
        if m.flags & MF_STRIP:
            n = m.num_faces + 2
            idx = struct.unpack_from('<%dH' % n, d, m.idx_ofs)
            tris = []
            for i in range(n - 2):
                a, b, c = idx[i], idx[i + 1], idx[i + 2]
                if a == b or b == c or a == c:
                    continue
                tris.append((a, b, c) if i % 2 == 0 else (b, a, c))
            return tris
        idx = struct.unpack_from('<%dH' % (m.num_faces * 3), d, m.idx_ofs)
        return [idx[i:i + 3] for i in range(0, len(idx), 3)]


def unpack_normpacked3(v):
    """Xbox D3DVSDT_NORMPACKED3: 11:11:10 signed."""
    def s(x, bits):
        if x & (1 << (bits - 1)):
            x -= 1 << bits
        return x / float((1 << (bits - 1)) - 1)
    return (s(v & 0x7ff, 11), s((v >> 11) & 0x7ff, 11), s((v >> 22) & 0x3ff, 10))


if __name__ == '__main__':
    import sys
    c = CMM(sys.argv[1])
    print('version %d  meshes %d  textures %d  nodes %d  bones %d  flags %#x  size %d'
          % (c.version, c.num_meshes, c.num_textures, c.num_nodes, c.num_bones, c.flags, len(c.data)))
    for m in c.meshes:
        print('mesh %2d  verts %5d faces %5d vtype %d flags %#07x tex %s node %d  v@%#x i@%#x'
              % (m.index, m.num_verts, m.num_faces, m.vtype, m.flags, m.tex, m.node,
                 m.vert_ofs or 0, m.idx_ofs))
    print('mesh data ends at %#x' % c.end_of_mesh_data)
