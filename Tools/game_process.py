"""Shared plumbing for reading the running game.

CoDWaW.exe on disk is Steam DRM packed: the .bind section decrypts .text at
load, so reading bytes from the file returns garbage no matter how correct the
PE section maths is. The only honest source for anything about this game is the
running process, which is why these tools exist at all.

The module has no ASLR and loads at 0x00400000, which is what makes the
hardcoded absolute addresses throughout WaWDll work.
"""
import ctypes
import ctypes.wintypes as w
import subprocess
import sys

PROCESS_VM_READ = 0x0010
PROCESS_QUERY_INFORMATION = 0x0400

MEM_COMMIT = 0x1000
PAGE_GUARD = 0x100
# Any protection constant with a readable component set.
READABLE = 0xEE

k32 = ctypes.WinDLL("kernel32", use_last_error=True)


class MEMORY_BASIC_INFORMATION(ctypes.Structure):
    _fields_ = [
        ("BaseAddress", ctypes.c_void_p),
        ("AllocationBase", ctypes.c_void_p),
        ("AllocationProtect", w.DWORD),
        ("RegionSize", ctypes.c_size_t),
        ("State", w.DWORD),
        ("Protect", w.DWORD),
        ("Type", w.DWORD),
    ]


class Game(object):
    """An open handle to the running game, with region enumeration and reads."""

    def __init__(self, image="CoDWaW.exe"):
        self.pid = self._find_pid(image)
        if not self.pid:
            sys.exit("%s is not running. Start the game first." % image)

        self.handle = k32.OpenProcess(
            PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, False, self.pid)
        if not self.handle:
            sys.exit("OpenProcess failed: %d. Try an elevated shell."
                     % ctypes.get_last_error())

        self._regions = None

    @staticmethod
    def _find_pid(image):
        out = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq %s" % image, "/FO", "CSV", "/NH"],
            capture_output=True, text=True).stdout
        for line in out.splitlines():
            if image in line:
                return int(line.split('","')[1])
        return None

    def read(self, addr, size):
        """Returns bytes, or None if the address is not readable."""
        buf = ctypes.create_string_buffer(size)
        got = ctypes.c_size_t(0)
        ok = k32.ReadProcessMemory(
            self.handle, ctypes.c_void_p(addr), buf, size, ctypes.byref(got))
        return buf.raw[:got.value] if ok else None

    def regions(self):
        """Committed, readable, non-guard regions. Enumerated once and cached."""
        if self._regions is not None:
            return self._regions

        self._regions = []
        mbi = MEMORY_BASIC_INFORMATION()
        addr = 0x10000
        while addr < 0x7FFF0000:
            if not k32.VirtualQueryEx(self.handle, ctypes.c_void_p(addr),
                                      ctypes.byref(mbi), ctypes.sizeof(mbi)):
                break
            base = mbi.BaseAddress or 0
            size = mbi.RegionSize
            if size == 0:
                break
            if (mbi.State == MEM_COMMIT and mbi.Protect & READABLE
                    and not mbi.Protect & PAGE_GUARD):
                self._regions.append((base, size))
            addr = base + size
        return self._regions

    def scan(self, needle):
        """Every address holding needle. Chunked, so a big region is fine."""
        hits = []
        for base, size in self.regions():
            off = 0
            while off < size:
                chunk = self.read(base + off, min(0x100000, size - off))
                if not chunk:
                    break
                i = chunk.find(needle)
                while i != -1:
                    hits.append(base + off + i)
                    i = chunk.find(needle, i + 1)
                # Overlap by len(needle)-1 so a match straddling a chunk
                # boundary is not missed.
                off += max(1, len(chunk) - len(needle) + 1)
        return hits
