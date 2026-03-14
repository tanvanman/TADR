"""Capture the TA window to a file using only ctypes (no extra deps)."""
import ctypes
import ctypes.wintypes as wt
import sys
import struct
import zlib

k32  = ctypes.windll.kernel32
u32  = ctypes.windll.user32
gdi  = ctypes.windll.gdi32

# -------------------------------------------------------------------
# Find the TA window
# -------------------------------------------------------------------
def find_window(class_name=None, title_substr=None):
    found = []
    EnumWindowsProc = ctypes.WINFUNCTYPE(ctypes.c_bool, wt.HWND, wt.LPARAM)
    buf = ctypes.create_unicode_buffer(512)

    def cb(hwnd, _):
        if not u32.IsWindowVisible(hwnd):
            return True
        u32.GetWindowTextW(hwnd, buf, len(buf))
        title = buf.value
        if title_substr and title_substr.lower() not in title.lower():
            return True
        if class_name:
            cls = ctypes.create_unicode_buffer(256)
            u32.GetClassNameW(hwnd, cls, len(cls))
            if cls.value != class_name:
                return True
        found.append((hwnd, title))
        return True

    u32.EnumWindows(EnumWindowsProc(cb), 0)
    return found


# -------------------------------------------------------------------
# Capture a HWND to raw BGRA bytes
# -------------------------------------------------------------------
def capture_hwnd(hwnd):
    rect = wt.RECT()
    u32.GetWindowRect(hwnd, ctypes.byref(rect))
    w = rect.right  - rect.left
    h = rect.bottom - rect.top

    hwnd_dc  = u32.GetDC(hwnd)
    mem_dc   = gdi.CreateCompatibleDC(hwnd_dc)
    bitmap   = gdi.CreateCompatibleBitmap(hwnd_dc, w, h)
    gdi.SelectObject(mem_dc, bitmap)

    # Use PrintWindow to capture even if partially obscured
    PW_RENDERFULLCONTENT = 0x00000002
    u32.PrintWindow(hwnd, mem_dc, PW_RENDERFULLCONTENT)

    class BITMAPINFOHEADER(ctypes.Structure):
        _fields_ = [('biSize',          wt.DWORD),
                    ('biWidth',         wt.LONG),
                    ('biHeight',        wt.LONG),
                    ('biPlanes',        wt.WORD),
                    ('biBitCount',      wt.WORD),
                    ('biCompression',   wt.DWORD),
                    ('biSizeImage',     wt.DWORD),
                    ('biXPelsPerMeter', wt.LONG),
                    ('biYPelsPerMeter', wt.LONG),
                    ('biClrUsed',       wt.DWORD),
                    ('biClrImportant',  wt.DWORD)]

    bmi = BITMAPINFOHEADER()
    bmi.biSize        = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.biWidth       = w
    bmi.biHeight      = -h   # top-down
    bmi.biPlanes      = 1
    bmi.biBitCount    = 32
    bmi.biCompression = 0    # BI_RGB

    buf_size = w * h * 4
    raw = (ctypes.c_uint8 * buf_size)()
    gdi.GetDIBits(mem_dc, bitmap, 0, h, raw, ctypes.byref(bmi), 0)

    gdi.DeleteObject(bitmap)
    gdi.DeleteDC(mem_dc)
    u32.ReleaseDC(hwnd, hwnd_dc)

    return bytes(raw), w, h


# -------------------------------------------------------------------
# Write a minimal PNG (no PIL needed)
# -------------------------------------------------------------------
def write_png(path, bgra_bytes, w, h):
    def chunk(tag, data):
        c = zlib.crc32(tag + data) & 0xFFFFFFFF
        return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', c)

    png_sig = b'\x89PNG\r\n\x1a\n'
    ihdr = chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))

    # Convert BGRA → RGB rows with filter byte 0
    rows = bytearray()
    for y in range(h):
        rows.append(0)  # filter type None
        for x in range(w):
            i = (y * w + x) * 4
            rows += bgra_bytes[i+2:i+3]  # R
            rows += bgra_bytes[i+1:i+2]  # G
            rows += bgra_bytes[i+0:i+1]  # B

    idat = chunk(b'IDAT', zlib.compress(bytes(rows), 6))
    iend = chunk(b'IEND', b'')

    with open(path, 'wb') as f:
        f.write(png_sig + ihdr + idat + iend)
    print(f"Saved {w}x{h} PNG → {path}")


# -------------------------------------------------------------------
# Main
# -------------------------------------------------------------------
output = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\Alex\AppData\Local\Temp\ta_screenshot.png'

windows = find_window(title_substr='Total Annihilation')
if not windows:
    # Try by process — grab any visible window from TotalA.exe
    windows = find_window(title_substr='TotalA')
if not windows:
    # Last resort: all visible top-level windows with non-empty titles
    windows = find_window()
    windows = [(h, t) for h, t in windows if t.strip()]

print("Visible windows found:", [(t,) for _, t in windows[:10]])

# Pick the first TA-related window, or the largest visible window
hwnd = None
for h, t in windows:
    if 'total' in t.lower() or 'annihil' in t.lower():
        hwnd = h
        print(f"Using window: {t!r} (hwnd={hwnd})")
        break

if not hwnd and windows:
    hwnd, title = windows[0]
    print(f"Falling back to: {title!r} (hwnd={hwnd})")

if not hwnd:
    print("No window found!")
    sys.exit(1)

raw, w, h = capture_hwnd(hwnd)
write_png(output, raw, w, h)
