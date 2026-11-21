"""Read the fixed-function D3D state of one mesh draw in a running race (xemu GDB stub on tcp 1234).

usage: python -I ffprobe.py [verts=f0] [hits=1] [addr=b78c8]
Breaks at cmm_draw_mesh's fixed-function draw point (0xb78c8: a mesh without an effect, mesh +0x40 = its FVF; EBX =
the mesh record, [EBP + 8] = the model) until a mesh with that vertex count (+0x08, hex) is drawn, then prints the
state caches the D3D library reads at the draw: lighting and material sources, the ambient, light 0, blending and the
alpha test, texture stages 0 / 1 (Xbox D3DTOP / D3DTA numbering), the texgen and texture matrix, the vertex shader's
colour flags. The mega-up's cage (objects\\megaup): obj19 = f0 (240 vertices), obj20 = e4 (228). docs\\models\\README.md
"The mega-up's cage in the race" says what each value should be.
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qrdbg import Gdb

DRAW = 0xb78c8
RENDER_STATES = [   # D3D render state cache (0x1188b8.. in the Xbox D3D deferred order)
    ('LIGHTING', 0x1188b8), ('SPECULARENABLE', 0x1188bc), ('LOCALVIEWER', 0x1188c0), ('COLORVERTEX', 0x1188c4),
    ('BACKSPECULARMATERIALSOURCE', 0x1188c8), ('BACKDIFFUSEMATERIALSOURCE', 0x1188cc),
    ('BACKAMBIENTMATERIALSOURCE', 0x1188d0), ('BACKEMISSIVEMATERIALSOURCE', 0x1188d4),
    ('SPECULARMATERIALSOURCE', 0x1188d8), ('DIFFUSEMATERIALSOURCE', 0x1188dc),
    ('AMBIENTMATERIALSOURCE', 0x1188e0), ('EMISSIVEMATERIALSOURCE', 0x1188e4),
    ('BACKAMBIENT', 0x1188e8), ('AMBIENT', 0x1188ec),
    ('ALPHAFUNC (0x4033c)', 0x118808), ('BLENDENABLE (0x40304)', 0x11880c), ('ALPHATESTENABLE (0x40300)', 0x118810),
    ('ALPHAREF (0x40340)', 0x118814), ('SRCBLEND (0x40344)', 0x118818), ('DESTBLEND (0x40348)', 0x11881c),
    ('BLENDOP (0x40350)', 0x118848),
]
GAME_CACHES = [
    ('light 0 colour 0x50e2f8', 0x50e2f8), ('ambient cache 0x50e2f4', 0x50e2f4), ('scene ambient 0x298da4', 0x298da4),
    ('blend descriptor 0x50e32c', 0x50e32c), ('lighting cache 0x50e324 (byte)', 0x50e324),
    ('TEXCOORDINDEX 0..3 0x50e364', 0x50e364), ('stage 0 slot 0x50e374', 0x50e374),
]
STAGE_STATES = [   # deferred texture stage states, 0x118520 + 0x80 x stage
    ('ADDRESSU', 0x00), ('ADDRESSV', 0x04), ('MAGFILTER', 0x0c), ('MINFILTER', 0x10), ('MIPFILTER', 0x14),
    ('COLORSIGN', 0x28), ('COLOROP', 0x30), ('COLORARG1', 0x38), ('COLORARG2', 0x3c), ('ALPHAOP', 0x40),
    ('ALPHAARG1', 0x48), ('ALPHAARG2', 0x4c), ('TEXTURETRANSFORMFLAGS', 0x54),
]
LIGHT0 = 0x292d48 + 0x60b4   # the game's D3DLIGHT8 for light 0 (0xa82d0 / 0xa8140 -> SetLight(0, this))


def dword(g, addr):
    return struct.unpack('<I', g.read(addr, 4))[0]


def floats(g, addr, count):
    return struct.unpack('<%df' % count, g.read(addr, 4 * count))


def step_off(g, addr):
    """Leave the breakpoint just hit: remove it, single-step, put it back."""
    g.cmd('z0,%x,1' % addr)
    g.cmd('s', 5)
    g.cmd('Z0,%x,1' % addr)


def wait_for_mesh(g, addr, verts):
    """Continue to the draw point until the mesh drawn has the vertex count; returns the registers there."""
    while True:
        g.cont()
        g._recv(30)
        regs = g.regs()
        if dword(g, regs['ebx'] + 8) == verts:
            return regs
        step_off(g, addr)


def print_mesh(g, mesh):
    surface = dword(g, mesh + 0x20)
    print('mesh %#x: flags %#x, FVF %#x, stages %d' % (mesh, dword(g, mesh + 0xc), dword(g, mesh + 0x40),
                                                        dword(g, mesh + 0x34)))
    print('  surface: setup %d, blend %#010x, flags %#x' % (dword(g, surface + 4), dword(g, surface + 8),
                                                            dword(g, surface + 0xc)))


def print_states(g):
    for name, addr in RENDER_STATES + GAME_CACHES:
        print('  %-34s %#010x' % (name, dword(g, addr)))
    for stage in range(2):
        base = 0x118520 + 0x80 * stage
        print('  stage %d: %s' % (stage, ', '.join('%s %#x' % (n, dword(g, base + o)) for n, o in STAGE_STATES)))


def print_light_and_shader(g):
    light = floats(g, LIGHT0 + 4, 12)
    print('  light 0 type %d diffuse %s specular %s ambient %s' % (
        dword(g, LIGHT0), ['%.3f' % v for v in light[0:4]], ['%.3f' % v for v in light[4:8]],
        ['%.3f' % v for v in light[8:12]]))
    print('  light 0 direction %s' % ['%.3f' % v for v in floats(g, LIGHT0 + 0x40, 3)])
    shader = dword(g, dword(g, 0x118510) + 0x380)
    print('  vertex shader %#x flags %#x (0x400 has diffuse, 0x800 has specular: 0x10f240 drops the colour'
          ' material sources the shader has no colour for)' % (shader, dword(g, shader + 4)))
    matrix = floats(g, 0x50e120, 16)
    print('  texture matrix 0x50e120: %s' % ' | '.join(' '.join('%.3f' % v for v in matrix[r * 4:r * 4 + 4])
                                                        for r in range(4)))


def main():
    verts = int(sys.argv[1], 16) if len(sys.argv) > 1 else 0xf0
    hits = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    addr = int(sys.argv[3], 16) if len(sys.argv) > 3 else DRAW
    g = Gdb()
    if g.cmd('Z0,%x,1' % addr) != 'OK':
        raise SystemExit('breakpoint failed at %#x' % addr)
    try:
        for _ in range(hits):
            regs = wait_for_mesh(g, addr, verts)
            print_mesh(g, regs['ebx'])
            print_states(g)
            print_light_and_shader(g)
            step_off(g, addr)
    finally:
        g.cmd('z0,%x,1' % addr)
        g.cont()


if __name__ == '__main__':
    main()
