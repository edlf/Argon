"""Disassemble XBE code at a virtual address (Ghidra address) with Capstone.
usage: python -I disasm.py <va_hex> [count=60] [xbe=game_data\\default.xbe, the original USA XBE]
Stops early at RET (unless count is given explicitly with a trailing '!').
"""
import os, sys
import capstone

here = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(here, '..', 'xbe'))
import xbe as xbe_file  # tools\xbe\xbe.py

xbe = sys.argv[3] if len(sys.argv) > 3 else os.path.join(here, '..', '..', 'game_data', 'default.xbe')
d = open(xbe, 'rb').read()


def off(v):
    o = xbe_file.va2off(d, v)
    if o is None:
        raise ValueError('address %#x not in a section' % v)
    return o


va = int(sys.argv[1], 16)
arg = sys.argv[2] if len(sys.argv) > 2 else '60'
force = arg.endswith('!')
count = int(arg.rstrip('!'))
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
o = off(va)
n = 0
for ins in md.disasm(d[o:o + count * 15], va):
    print('%08x  %-8s %s' % (ins.address, ins.mnemonic, ins.op_str))
    n += 1
    if n >= count or (ins.mnemonic == 'ret' and not force):
        break
