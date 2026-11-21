"""Print an Xbox pixel shader (.xpu: "PSB0" + D3DPIXELSHADERDEF, NV2A register combiners) as readable equations.
usage: python -I xpu.py <file.xpu>
"""
import struct
import sys

REGISTERS = {0: 'zero', 1: 'c0', 2: 'c1', 3: 'fog', 4: 'v0', 5: 'v1', 8: 't0', 9: 't1', 0xa: 't2', 0xb: 't3',
             0xc: 'r0', 0xd: 'r1', 0xe: 'v1r0sum', 0xf: 'ef'}
MAPPINGS = {0: '{}', 1: '(1-{})', 2: '(2*{}-1)', 3: '-(2*{}-1)', 4: '({}-0.5)', 5: '-({}-0.5)', 6: '{}', 7: '-{}'}
OUTPUT_SCALES = {0: '', 1: ' - 0.5 (bias)', 2: ' x2', 3: ' x2 - 1 (bias, x2)', 4: ' x4', 6: ' / 2'}
TEXTURE_MODES = {0: 'none', 1: 'project2d', 2: 'project3d', 3: 'cubemap', 4: 'passthru', 5: 'clipplane',
                 6: 'bumpenv', 7: 'bumpenv_lum', 8: 'brdf', 9: 'dot_st', 0xa: 'dot_zw', 0xb: 'dot_rflct_diff',
                 0xc: 'dot_rflct_spec', 0xd: 'dot_stq', 0xe: 'dpndnt_ar', 0xf: 'dpndnt_gb', 0x10: 'dotproduct',
                 0x11: 'dot_rflct_spec_const'}


def describe_input(byte, alpha_portion):
    register = REGISTERS.get(byte & 0xf, 'reg%x' % (byte & 0xf))
    if byte & 0xf == 0 and byte & 0xe0 == 0x20:
        return '1'
    if byte & 0xf == 0 and byte & 0xe0 == 0:
        return '0'
    channel = '.a' if byte & 0x10 else ('.b' if alpha_portion else '.rgb')
    return MAPPINGS[byte >> 5].format(register + channel)


def describe_inputs(word, alpha_portion):
    return [describe_input((word >> shift) & 0xff, alpha_portion) for shift in (24, 16, 8, 0)]


def describe_outputs(word, inputs, alpha_portion):
    a, b, c, d = inputs
    cd_out, ab_out, sum_out = word & 0xf, (word >> 4) & 0xf, (word >> 8) & 0xf
    product = '.' if alpha_portion else '*'
    ab_op = ' dot ' if word & 0x2000 else ' * '
    cd_op = ' dot ' if word & 0x1000 else ' * '
    lines = []
    if ab_out:
        lines.append('%s = %s%s%s' % (REGISTERS[ab_out], a, ab_op, b))
    if cd_out:
        lines.append('%s = %s%s%s' % (REGISTERS[cd_out], c, cd_op, d))
    if sum_out:
        joiner = ' mux ' if word & 0x4000 else ' + '
        lines.append('%s = %s%s%s%s%s' % (REGISTERS[sum_out], a, ab_op.strip() and ab_op, b, joiner, c + cd_op + d))
    scale = OUTPUT_SCALES.get((word >> 15) & 7, ' scale?')
    flags = []
    if word & 0x40000:
        flags.append('AB blue->alpha')
    if word & 0x80000:
        flags.append('CD blue->alpha')
    del product
    return lines, scale, flags


def print_stage(index, rgb_in, rgb_out, alpha_in, alpha_out, c0, c1):
    print('stage %d   (c0 %08x, c1 %08x)' % (index, c0, c1))
    for name, word_in, word_out, is_alpha in (('rgb', rgb_in, rgb_out, False), ('alpha', alpha_in, alpha_out, True)):
        inputs = describe_inputs(word_in, is_alpha)
        lines, scale, flags = describe_outputs(word_out, inputs, is_alpha)
        for line in lines:
            print('  %-5s %s%s %s' % (name, line, scale, ' '.join(flags)))


def print_final(abcd, efg, const0, const1):
    a, b, c, d = describe_inputs(abcd, False)
    e, f, g, _ = describe_inputs(efg, False)
    print('final  (c0 %08x, c1 %08x, flags %02x)' % (const0, const1, efg & 0xff))
    print('  ef  = %s * %s' % (e, f))
    print('  v1r0sum = v1 + r0')
    print('  rgb = %s * %s + (1 - %s) * %s + %s' % (a, b, a, c, d))
    print('  a   = %s' % g)


def main():
    data = open(sys.argv[1], 'rb').read()
    if data[:4] != b'PSB0':
        raise SystemExit('not a PSB0 file')
    words = struct.unpack_from('<60I', data, 4)
    alpha_in, final_abcd, final_efg = words[0:8], words[8], words[9]
    const0, const1, alpha_out, rgb_in = words[10:18], words[18:26], words[26:34], words[34:42]
    final_c0, final_c1, rgb_out = words[43], words[44], words[45:53]
    count, texture_modes, dot_mapping, input_texture = words[53], words[54], words[55], words[56]
    print('combiners %d (flags %06x), texture modes: %s' % (
        count & 0xff, count & ~0xff, ', '.join('t%d %s' % (i, TEXTURE_MODES.get((texture_modes >> 5 * i) & 0x1f, '?'))
                                               for i in range(4))))
    print('dot mapping %08x, input texture %08x, compare %08x, C0 map %08x, C1 map %08x, final consts %08x' % (
        dot_mapping, input_texture, words[42], words[57], words[58], words[59]))
    for stage in range(count & 0xff):
        print_stage(stage, rgb_in[stage], rgb_out[stage], alpha_in[stage], alpha_out[stage], const0[stage],
                    const1[stage])
    print_final(final_abcd, final_efg, final_c0, final_c1)


if __name__ == '__main__':
    main()
