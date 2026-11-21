"""Write-watchpoint on a guest address (xemu GDB stub on tcp 1234; start xemu with run_xemu.ps1 -Wait to catch
writes from boot). Prints EIP, the caller (return address on the stack) and the new value on every hit.
usage: python -I watch.py <addr hex> [len=4] [hits=20] [timeout_s=180] [dump=0x54]
"""
import os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from qrdbg import Gdb


def main():
    addr = int(sys.argv[1], 16)
    n = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    hits = int(sys.argv[3]) if len(sys.argv) > 3 else 20
    tmo = float(sys.argv[4]) if len(sys.argv) > 4 else 180
    dump = int(sys.argv[5], 16) if len(sys.argv) > 5 else 0x54
    g = Gdb()
    r = g.cmd('Z2,%x,%x' % (addr, n))
    if r != 'OK':
        raise SystemExit('watchpoint failed: %r' % r)
    try:
        for i in range(hits):
            g.cont()
            try:
                stop = g._recv(tmo)
            except Exception:
                g.halt()
                print('no write within %ss' % tmo)
                break
            regs = g.regs()
            ret = struct.unpack('<I', g.read(regs['esp'], 4))[0]
            vals = struct.unpack('<%di' % (dump // 4), g.read(addr, dump))
            print('hit %d: %s eip %#x ret? %#x  values %s' % (i, stop, regs['eip'], ret, list(vals)))
            if any(vals):
                break
    finally:
        g.cmd('z2,%x,%x' % (addr, n))
        g.cont()


if __name__ == '__main__':
    main()
