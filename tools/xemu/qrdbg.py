"""Tiny GDB remote-protocol client for xemu (started with -s -> tcp 1234).

usage: python -I qrdbg.py <command> [args]
  regs                         halt, print registers, resume
  rd <addr> [len]              hex dump guest memory (default 64 bytes)
  rf <addr> [count]            read float32 values
  wr <addr> <hexbytes>         write raw bytes, e.g. wr 344d0 6800002044
  wf <addr> <float> [...]      write float32 values
  diff <old.xbe> <new.xbe>     write every byte that differs between two XBEs into guest memory
  bp <addr> [timeout_s]        break at addr, wait for hit, print regs + stack, remove bp, resume
  halt / cont                  stop / resume the VM
Addresses are hex guest virtual addresses (= XBE / Ghidra addresses).
The VM is always resumed afterwards unless the command is 'halt'.
"""
import os, socket, struct, sys, time

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'xbe'))
HOST, PORT = '127.0.0.1', int(os.environ.get('QRDBG_PORT', '1234'))
REGS = ['eax', 'ecx', 'edx', 'ebx', 'esp', 'ebp', 'esi', 'edi', 'eip', 'eflags']


class Gdb:
    def __init__(self):
        self.s = socket.create_connection((HOST, PORT), timeout=5)
        self.buf = b''
        self.s.sendall(b'+')
        self.cmd('QStartNoAckMode')
        self.noack = True
        self.halt()

    # --- packet layer ---
    def _send(self, data):
        cs = sum(data.encode()) & 0xff
        self.s.sendall(b'$' + data.encode() + b'#%02x' % cs)

    def _recv(self, timeout=5):
        self.s.settimeout(timeout)
        while True:
            i = self.buf.find(b'$')
            j = self.buf.find(b'#', i + 1) if i >= 0 else -1
            if i >= 0 and j >= 0 and len(self.buf) >= j + 3:
                pkt = self.buf[i + 1:j].decode('latin-1')
                self.buf = self.buf[j + 3:]
                if not getattr(self, 'noack', False):
                    self.s.sendall(b'+')
                return pkt
            chunk = self.s.recv(65536)
            if not chunk:
                raise ConnectionError('gdb stub closed')
            self.buf += chunk

    def cmd(self, data, timeout=5):
        self._send(data)
        return self._recv(timeout)

    # --- run control ---
    def halt(self):
        self.s.sendall(b'\x03')
        try:
            return self._recv(2)
        except socket.timeout:
            return None  # already stopped

    def cont(self):
        self._send('c')

    # --- memory / regs ---
    def read(self, addr, n):
        out = b''
        while n:
            k = min(n, 0x800)
            r = self.cmd('m%x,%x' % (addr, k))
            if r.startswith('E'):
                raise IOError('read %#x failed: %s' % (addr, r))
            out += bytes.fromhex(r); addr += k; n -= k
        return out

    def write(self, addr, data):
        for i in range(0, len(data), 0x400):
            part = data[i:i + 0x400]
            r = self.cmd('M%x,%x:%s' % (addr + i, len(part), part.hex()))
            if r != 'OK':
                raise IOError('write %#x failed: %s' % (addr + i, r))

    def regs(self):
        r = bytes.fromhex(self.cmd('g'))
        return dict(zip(REGS, struct.unpack_from('<10I', r)))

    def bp(self, addr, timeout):
        r = self.cmd('Z0,%x,1' % addr)
        if r != 'OK':
            r = self.cmd('Z1,%x,1' % addr)  # hardware fallback
        if r != 'OK':
            raise IOError('breakpoint failed: %s' % r)
        self.cont()
        try:
            stop = self._recv(timeout)
        except socket.timeout:
            self.halt(); stop = None
        regs = self.regs()
        self.cmd('z0,%x,1' % addr); self.cmd('z1,%x,1' % addr)
        return stop, regs


def hexdump(addr, data):
    for i in range(0, len(data), 16):
        print('%08x  %s' % (addr + i, data[i:i + 16].hex(' ')))


def main():
    if len(sys.argv) < 2:
        print(__doc__); return
    c, a = sys.argv[1], sys.argv[2:]
    g = Gdb()
    try:
        if c == 'regs':
            print(' '.join('%s=%08x' % kv for kv in g.regs().items()))
        elif c == 'rd':
            addr = int(a[0], 16); hexdump(addr, g.read(addr, int(a[1], 0) if len(a) > 1 else 64))
        elif c == 'rf':
            addr = int(a[0], 16); n = int(a[1]) if len(a) > 1 else 1
            for i, v in enumerate(struct.unpack('<%df' % n, g.read(addr, 4 * n))):
                print('%08x  %g' % (addr + 4 * i, v))
        elif c == 'wr':
            g.write(int(a[0], 16), bytes.fromhex(''.join(a[1:]))); print('ok')
        elif c == 'wf':
            g.write(int(a[0], 16), struct.pack('<%df' % (len(a) - 1), *map(float, a[1:]))); print('ok')
        elif c == 'diff':
            import xbe
            old, new = open(a[0], 'rb').read(), open(a[1], 'rb').read()
            i, n = 0, 0
            while i < len(new):
                if old[i] != new[i]:
                    j = i
                    while j < len(new) and old[j] != new[j]:
                        j += 1
                    va = xbe.off2va(new, i)
                    if va is None:
                        print('skip header/unmapped bytes at file %#x' % i)
                    else:
                        g.write(va, new[i:j]); n += 1
                        print('wrote %d bytes at %08x' % (j - i, va))
                    i = j
                else:
                    i += 1
            print('%d ranges written' % n)
        elif c == 'bp':
            addr = int(a[0], 16)
            stop, regs = g.bp(addr, float(a[1]) if len(a) > 1 else 30)
            print('hit' if stop else 'timeout', stop)
            print(' '.join('%s=%08x' % kv for kv in regs.items()))
            if stop:
                st = g.read(regs['esp'], 48)
                print('stack:', ' '.join('%08x' % v for v in struct.unpack('<12I', st)))
        elif c == 'halt':
            print('halted'); return
        elif c != 'cont':
            print(__doc__)
    finally:
        if c != 'halt':
            g.cont()
        g.s.close()


if __name__ == '__main__':
    main()
